using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Security.Cryptography.Xml;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using LotusEditor.GameProject;
using LotusEditor.Utility;

namespace LotusEditor.GameDev
{
    enum BuildConfiguration
    {
        DEBUG,
        DEBUG_DLL,
        RELEASE,
        RELEASE_DLL
    }

    static class VisualStudio
    {

        private static EnvDTE80.DTE2 _vsInstance = null;

        private static readonly ManualResetEventSlim _resetEvent = new(false);
        private static readonly string _progId = "VisualStudio.DTE.17.0";
        private static readonly object _lock = new();
        public static bool BuildSucceeded { get; private set; } = true;
        public static bool BuildFinished { get; private set; } = true;

        private static readonly string[] _configNames = new[] { "Debug", "DebugDll", "Release", "ReleaseDll" };

        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable pprot);

        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(uint reserved, out IBindCtx ppbc);

        public static string GetConfigName(BuildConfiguration config) => _configNames[(int)config];

        private static void CallOnSTAThread(Action action)
        {
            Debug.Assert(action != null);
            var thread = new Thread(() =>
            {
                MessageFilter.Register();
                try { action(); }
                catch (Exception ex) { Logger.Warn(ex.Message); }
                finally { MessageFilter.Revoke(); }
            });

            thread.SetApartmentState(ApartmentState.STA);
            thread.Start();
            thread.Join();
        }

        public static void OpenInstance(string slnPath)
        {
            lock (_lock)
            {
                OpenInstanceInternal(slnPath);
            }
        }

        private static void OpenInstanceInternal(string slnPath)
        {
            IRunningObjectTable table = null;
            IEnumMoniker moniker = null;
            IBindCtx bindCtx = null;
            try
            {
                if (_vsInstance == null)
                {

                    Logger.Info($"Attempting to open Visual Studio with solution {slnPath}");
                    // find vs instance
                    var hres = GetRunningObjectTable(0, out table);
                    if (hres < 0 || table == null)
                        throw new COMException($"GetRunningObjectTable() returned HRESULT: {hres:X8}");

                    table.EnumRunning(out moniker);
                    moniker.Reset();

                    hres = CreateBindCtx(0, out bindCtx);
                    if (hres < 0 || bindCtx == null)
                        throw new COMException($"CreateBindCtx() returned HRESULT: {hres:X8}");

                    var currentMoniker = new IMoniker[1];

                    while (moniker.Next(1, currentMoniker, IntPtr.Zero) == 0)
                    {
                        var name = string.Empty;
                        currentMoniker[0]?.GetDisplayName(bindCtx, null, out name);
                        if (!name.Contains(_progId)) continue;
                        hres = table.GetObject(currentMoniker[0], out object obj);
                        if (hres < 0 || obj == null)
                            throw new COMException($"RunningObjectTable GetObject returned HRESULT: {hres:X8}");
                        var dte = obj as EnvDTE80.DTE2;
                        var slnName = dte.Solution.FullName;
                        if (slnName != slnPath) continue;
                        _vsInstance = dte;
                        break;
                    }
                }

                if (_vsInstance == null)
                {
                    var visualStudioType = Type.GetTypeFromProgID(_progId, true);
                    _vsInstance = Activator.CreateInstance(visualStudioType) as EnvDTE80.DTE2;

                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Error("Failed to open Visual Studio");
            }
            finally
            {
                if (moniker != null) Marshal.ReleaseComObject(moniker);
                if (table != null) Marshal.ReleaseComObject(table);
                if (bindCtx != null) Marshal.ReleaseComObject(bindCtx);
            }

            Logger.Info("Visual Studio instance successfully opened");

        }


        public static void CloseInstance()
        {
            lock (_lock)
            {
                CloseInstanceInternal();
            }
        }

        private static void CloseInstanceInternal()
        {
            CallOnSTAThread(() =>
            {
                if (_vsInstance?.Solution.IsOpen == true)
                {
                    Logger.Info("Saving and closing Visual Studio solution");
                    _vsInstance.ExecuteCommand("File.SaveAll");
                    _vsInstance.Solution.Close(true);
                }
                Logger.Info("Exiting Visual Studio instance");
                _vsInstance?.Quit();
                _vsInstance = null;
            });

        }

        public static bool AddFilesToSolution(string solution, string projName, string[] files)
        {
            lock (_lock)
            {
                return AddFilesToSolutionInternal(solution, projName, files);
            }
        }

        private static bool AddFilesToSolutionInternal(string solution, string projName, string[] files)
        {
            Debug.Assert(files?.Length > 0);
            OpenInstanceInternal(solution);
            try
            {
                if (_vsInstance != null)
                {
                    CallOnSTAThread(() =>
                    {
                        if (!_vsInstance.Solution.IsOpen)
                        {
                            Logger.Info($"Opening Visual Studio solution {solution}");
                            _vsInstance.Solution.Open(solution);
                        }
                        else
                        {
                            Logger.Info($"Saving Visual Studio solution {solution}");
                            _vsInstance.ExecuteCommand("File.SaveAll");
                        }

                        foreach (EnvDTE.Project project in _vsInstance.Solution.Projects)
                        {
                            if (project is null || !project.UniqueName.Contains(projName)) continue;
                            foreach (var file in files)
                            {
                                Logger.Info($"Attempting to add {file} to {projName} in {solution}");
                                project.ProjectItems.AddFromFile(file);
                            }
                        }

                        var cpp = files.FirstOrDefault(x => Path.GetExtension(x) == ".cpp");
                        if (!string.IsNullOrEmpty(cpp))
                        {
                            Logger.Info($"Attempting to open {cpp} in the {solution} Visual Studio solution");
                            _vsInstance.ItemOperations.OpenFile(cpp, EnvDTE.Constants.vsViewKindTextView).Visible = true;
                        }

                        _vsInstance.MainWindow.Activate();
                        _vsInstance.MainWindow.Visible = true;
                    });

                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Error("Failed to add scripts to the Visual Studio project");
                return false;
            }

            return true;
        }

        private static void BuildSolutionInternal(Project project, BuildConfiguration buildConfig, bool showWindow)
        {
            if (IsDebuggingInternal())
            {
                Logger.Error("Visual Studio is currently running a process");
                return;
            }
            OpenInstanceInternal(project.SolutionName);
            BuildFinished = BuildSucceeded = false;

            CallOnSTAThread(() =>
            {
                if (!_vsInstance.Solution.IsOpen)
                {
                    Logger.Info($"Opening Visual Studio solution {project.SolutionName}");
                    _vsInstance.Solution.Open(project.SolutionName);
                }

                _vsInstance.MainWindow.Visible = showWindow;
                _vsInstance.Events.BuildEvents.OnBuildProjConfigBegin += OnBuildBegin;
                _vsInstance.Events.BuildEvents.OnBuildProjConfigDone += OnBuildDone;
            });

            var configName = GetConfigName(buildConfig);
            try
            {

                foreach (var pdb in Directory.GetFiles(Path.Combine($"{project.Path}", $@"x64\{configName}"),
                             "*.pdb"))
                {
                    File.Delete(pdb);
                }
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
            }
            CallOnSTAThread(() =>
                {
                    _vsInstance.Solution.SolutionBuild.SolutionConfigurations.Item(configName).Activate();
                    _vsInstance.ExecuteCommand("Build.BuildSolution");
                    _resetEvent.Wait();
                    _resetEvent.Reset();
                });

        }

        public static void BuildSolution(Project project, BuildConfiguration buildConfig, bool showWindow = true)
        {
            lock (_lock)
            {
                BuildSolutionInternal(project, buildConfig, showWindow);
            }
        }

        private static void OnBuildDone(string project, string projectconfig, string platform, string solutionconfig, bool success)
        {
            if (BuildFinished) return;

            if (success) Logger.Info($"Built {project} for {platform} in {projectconfig} mode successfully");
            else Logger.Error($"Failed to build {project} in {projectconfig} mode");

            BuildFinished = true;
            BuildSucceeded = success;
            _resetEvent.Set();
        }

        private static void OnBuildBegin(string project, string projectconfig, string platform, string solutionconfig)
        {
            if (BuildFinished) return;
            Logger.Info($"[MSVC] Building {project} for platform {platform}. Project Config: {projectconfig}. Solution Config: {solutionconfig}");
        }

        private static bool IsDebuggingInternal()
        {
            var res = false;
            CallOnSTAThread(() =>
            {
                res = _vsInstance != null && (_vsInstance.Debugger.CurrentProgram != null || _vsInstance.Debugger.CurrentMode == EnvDTE.dbgDebugMode.dbgRunMode);
            });

            return res;
        }

        public static bool IsDebugging()
        {
            lock (_lock)
            {
                return IsDebuggingInternal();
            }
        }

        private static void RunInternal(Project project, BuildConfiguration buildConfig, bool debug)
        {
            CallOnSTAThread(() =>
            {
                if (_vsInstance != null && !IsDebuggingInternal() && BuildSucceeded)
                    _vsInstance.ExecuteCommand(debug ? "Debug.Start" : "Debug.StartWithoutDebugging");

            });
        }

        public static void Run(Project project, BuildConfiguration buildConfig, bool debug)
        {
            lock (_lock)
            {
                RunInternal(project, buildConfig, debug);
            }
        }

        private static void StopInternal()
        {
            CallOnSTAThread(() =>
            {
                if (_vsInstance != null && IsDebuggingInternal())
                    _vsInstance.ExecuteCommand("Debug.StopDebugging");
            });
        }

        public static void Stop()
        {
            lock (_lock)
            {
                StopInternal();
            }
        }
    }

    // Class containing the IOleMEssageFilter thread error-handling function
    public class MessageFilter : IOleMessageFilter
    {
        private const int SERVERCALL_ISHANDLED = 0;
        private const int PENDINGMSG_WAITDEFPROCESS = 2;
        private const int SERVERCALL_RETRYLATER = 2;

        [DllImport("Ole32.dll")]
        private static extern int CoRegisterMessageFilter(IOleMessageFilter newFilter, out IOleMessageFilter oldFilter);

        public static void Register()
        {
            IOleMessageFilter newFilter = new MessageFilter();
            int hr = CoRegisterMessageFilter(newFilter, out var oldFilter);
            Debug.Assert(hr >= 0, "Registering COM IMessageFilter failed.");
        }

        public static void Revoke()
        {
            int hr = CoRegisterMessageFilter(null, out var oldFilter);
            Debug.Assert(hr >= 0, "Unregistering COM IMessageFilter failed.");
        }


        int IOleMessageFilter.HandleInComingCall(int dwCallType, System.IntPtr hTaskCaller, int dwTickCount, System.IntPtr lpInterfaceInfo)
        {
            //returns the flag SERVERCALL_ISHANDLED. 
            return SERVERCALL_ISHANDLED;
        }


        int IOleMessageFilter.RetryRejectedCall(System.IntPtr hTaskCallee, int dwTickCount, int dwRejectType)
        {
            // Thread call was refused, try again. 
            if (dwRejectType == SERVERCALL_RETRYLATER)
            {
                // retry thread call at once, if return value >=0 & <100. 
                Debug.WriteLine("COM server busy. Retrying call to EnvDTE interface.");
                return 500;
            }
            // Too busy. Cancel call.
            return -1;
        }


        int IOleMessageFilter.MessagePending(System.IntPtr hTaskCallee, int dwTickCount, int dwPendingType)
        {
            return PENDINGMSG_WAITDEFPROCESS;
        }
    }

    [ComImport(), Guid("00000016-0000-0000-C000-000000000046"),
    InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    interface IOleMessageFilter
    {

        [PreserveSig]
        int HandleInComingCall(int dwCallType, IntPtr hTaskCaller, int dwTickCount, IntPtr lpInterfaceInfo);


        [PreserveSig]
        int RetryRejectedCall(IntPtr hTaskCallee, int dwTickCount, int dwRejectType);


        [PreserveSig]
        int MessagePending(IntPtr hTaskCallee, int dwTickCount, int dwPendingType);
    }


}
