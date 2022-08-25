using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;
using System.Security.Cryptography.Xml;
using System.Text;
using System.Threading.Tasks;
using LotusEditor.GameProject;
using LotusEditor.Utility;

namespace LotusEditor.GameDev
{
    static class VisualStudio
    {

        private static EnvDTE80.DTE2 _vsInstance = null;

        private static readonly string _progId = "VisualStudio.DTE.17.0";
        public static bool BuildSucceeded { get; private set; } = true;
        public static bool BuildFinished { get; private set; } = true;

        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(uint reserved, out IRunningObjectTable pprot);

        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(uint reserved, out IBindCtx ppbc);


        public static void OpenInstance(string slnPath)
        {
            IRunningObjectTable table = null;
            IEnumMoniker moniker = null;
            IBindCtx bindCtx = null;
            try
            {
                if (_vsInstance != null) return;

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

                if (_vsInstance != null) return;

                var visualStudioType = Type.GetTypeFromProgID(_progId, true);
                _vsInstance = Activator.CreateInstance(visualStudioType) as EnvDTE80.DTE2;
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
            if (_vsInstance?.Solution.IsOpen == true)
            {
                Logger.Info("Saving and closing Visual Studio solution");
                _vsInstance.ExecuteCommand("File.SaveAll");
                _vsInstance.Solution.Close(true);
            }
            Logger.Info("Exiting Visual Studio instance");
            _vsInstance?.Quit();
        }

        public static bool AddFilesToSolution(string solution, string projName, string[] files)
        {
            Debug.Assert(files?.Length > 0);
            OpenInstance(solution);
            try
            {
                if (_vsInstance != null)
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
                        if (project.UniqueName.Contains(projName))
                        {
                            foreach (var file in files)
                            {
                                Logger.Info($"Attempting to add {file} to {projName} in {solution}");
                                project.ProjectItems.AddFromFile(file);
                            }
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

        public static void BuildSolution(Project project, string configName, bool showWindow)
        {
            if (IsDebugging())
            {
                Logger.Error("Visual Studio is currently running a process");
                return;
            }
            Logger.Info($"Building {project.Name}");

            OpenInstance(project.SolutionName);
            BuildFinished = BuildSucceeded = false;

            for (var i = 0; i < 3 && !BuildSucceeded; ++i)
            {
                try
                {
                    if (!_vsInstance.Solution.IsOpen)
                    {
                        Logger.Info($"Opening Visual Studio solution {project.SolutionName}");
                        _vsInstance.Solution.Open(project.SolutionName);
                    }

                    _vsInstance.MainWindow.Visible = showWindow;

                    _vsInstance.Events.BuildEvents.OnBuildProjConfigBegin += OnBuildBegin;
                    _vsInstance.Events.BuildEvents.OnBuildProjConfigDone += OnBuildDone;

                    try
                    {
                        foreach (var pdb in Directory.GetFiles(Path.Combine($"{project.Location}", $@"x64\{configName}"),
                                    "*.pdb"))
                        {
                            File.Delete(pdb);
                        }
                    }
                    catch (Exception e)
                    {
                        Debug.WriteLine(e.Message);
                    }



                    // Logger.Info($"Setting Visual Studio solution to {configName} mode");
                    _vsInstance.Solution.SolutionBuild.SolutionConfigurations.Item(configName).Activate();

                    _vsInstance.ExecuteCommand("Build.BuildSolution");
                }
                catch (Exception ex)
                {
                    Debug.WriteLine(ex.Message);
                    Logger.Error("Failed to build");
                    System.Threading.Thread.Sleep(1000);
                }
            }


        }

        private static void OnBuildDone(string project, string projectconfig, string platform, string solutionconfig, bool success)
        {
            if (BuildFinished) return;

            if (success) Logger.Info($"Built {project} for {platform} in {projectconfig} mode successfully");
            else Logger.Error($"Failed to build {project} in {projectconfig} mode");

            BuildFinished = true;
            BuildSucceeded = success;
        }

        private static void OnBuildBegin(string project, string projectconfig, string platform, string solutionconfig)
        {
            Logger.Info($"[MSVC] Building {project} for platform {platform}. Project Config: {projectconfig}. Solution Config: {solutionconfig}");
        }

        public static bool IsDebugging()
        {
            // if (_vsInstance == null) return false;
            var res = false;

            for (var i = 0; i < 3; i++)
            {
                try
                {
                    res = _vsInstance != null && (_vsInstance.Debugger.CurrentProgram != null || _vsInstance.Debugger.CurrentMode == EnvDTE.dbgDebugMode.dbgRunMode);
                }
                catch (Exception e)
                {
                    Debug.WriteLine(e.Message);
                    Logger.Error("Failed to check if the Visual Studio solution was running a process");
                    if (!res) System.Threading.Thread.Sleep(1000);
                }

                if (res) break;
            }


            return res;
        }
    }

}
