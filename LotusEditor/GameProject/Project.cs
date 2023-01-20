using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Markup;
using LotusEditor.Components;
using LotusEditor.DllWrapper;
using LotusEditor.GameDev;
using LotusEditor.Utility;

namespace LotusEditor.GameProject
{


    [DataContract(Name = "Game")]
    internal class Project : ViewModelBase
    {
        public static string Extension => ".lproj";
        [DataMember] public string Name { get; private set; } = "New Project";

        public string Location { get; private set; }
        public string FullPath => $"{Location}{Name}{Extension}";
        public string SolutionName => $@"{Location}{Name}.sln";
        public string ContentPath => $@"{Location}Assets\";



        private int _buildConfig;
        [DataMember]
        public int BuildConfig { get => _buildConfig; set { if (_buildConfig == value) return; _buildConfig = value; OnPropertyChanged(nameof(BuildConfig)); } }

        public BuildConfiguration ExeBuildConfig =>
            BuildConfig == 0 ? BuildConfiguration.DEBUG : BuildConfiguration.RELEASE;

        public BuildConfiguration DllBuildConfig =>
            BuildConfig == 0 ? BuildConfiguration.DEBUG_DLL : BuildConfiguration.RELEASE_DLL;

        private string[] _availableScripts;
        public string[] AvailableScripts { get => _availableScripts; private set { if (_availableScripts == value) return; _availableScripts = value; OnPropertyChanged(nameof(AvailableScripts)); } }

        [DataMember(Name = "Scenes")]
        private ObservableCollection<Scene> _scenes = new();
        public ReadOnlyObservableCollection<Scene> Scenes { get; private set; }
        private Scene _activeScene;

        public Scene ActiveScene
        {
            get => _activeScene;
            set
            {
                if (_activeScene == value) return;
                _activeScene = value;
                OnPropertyChanged(nameof(ActiveScene));
            }
        }


        public static Project Current => Application.Current.MainWindow?.DataContext as Project;


        public static History HistoryManager { get; } = new();
        public static History SelectionHistoryManager { get; } = new();

        public ICommand AddSceneCmd { get; private set; }
        public ICommand RemoveSceneCmd { get; private set; }

        public ICommand UndoCmd { get; private set; }
        public ICommand RedoCmd { get; private set; }

        public ICommand UndoSelectionCmd { get; private set; }
        public ICommand RedoSelectionCmd { get; private set; }

        public ICommand SaveCmd { get; private set; }
        public ICommand BuildCmd { get; private set; }
        public ICommand BuildExeCmd { get; private set; }

        public ICommand DebugStartCmd { get; private set; }
        public ICommand DebugStartWithoutDebuggingCmd { get; private set; }
        public ICommand DebugStopCmd { get; private set; }
        


        public Project(string name, string path)
        {
            Name = name;
            Location = path;

            OnDeserialized(new StreamingContext());
        }

        private void SetCommands()
        {
            AddSceneCmd = new RelayCommand<object>(x =>
            {
                AddSceneInternal($"New Scene {_scenes.Count}");
                var newScene = _scenes.Last();
                var index = _scenes.Count - 1;
                HistoryManager.AddUndoRedoAction(new UndoRedoAction(
                    $"Add Scene {newScene.Name}",
                    () => RemoveSceneInternal(newScene),
                    () => _scenes.Insert(index, newScene)));
            });

            RemoveSceneCmd = new RelayCommand<Scene>(x =>
            {
                var index = _scenes.IndexOf(x);
                RemoveSceneInternal(x);

                HistoryManager.AddUndoRedoAction(new UndoRedoAction($"Remove Scene {x.Name}",
                    () => _scenes.Insert(index, x),
                    () => RemoveSceneInternal(x)));
            }, x => !x.IsActive);

            UndoCmd = new RelayCommand<object>(x => HistoryManager.Undo(), x => HistoryManager.UndoList.Any());
            RedoCmd = new RelayCommand<object>(x => HistoryManager.Redo(), x => HistoryManager.RedoList.Any());
            SaveCmd = new RelayCommand<object>(x => Save(this));
            BuildCmd = new RelayCommand<bool>(async x => await BuildGameDll(x), x => !VisualStudio.IsDebugging() && VisualStudio.BuildFinished);
            BuildExeCmd = new RelayCommand<bool>(async x => await BuildGameExe(x), x => !VisualStudio.IsDebugging() && VisualStudio.BuildFinished);

            UndoSelectionCmd = new RelayCommand<object>(x => SelectionHistoryManager.Undo(), x => SelectionHistoryManager.UndoList.Any());
            RedoSelectionCmd = new RelayCommand<object>(x => SelectionHistoryManager.Redo(), x => SelectionHistoryManager.RedoList.Any());
            DebugStartCmd = new RelayCommand<object>(async x => await RunGame(true), x => !VisualStudio.IsDebugging() && VisualStudio.BuildFinished);
            DebugStartWithoutDebuggingCmd = new RelayCommand<object>(async x => await RunGame(false), x => !VisualStudio.IsDebugging() && VisualStudio.BuildFinished);
            DebugStopCmd = new RelayCommand<object>(async x => await StopGame(), x => VisualStudio.IsDebugging());

            OnPropertyChanged(nameof(AddSceneCmd));
            OnPropertyChanged(nameof(RemoveSceneCmd));
            OnPropertyChanged(nameof(UndoCmd));
            OnPropertyChanged(nameof(RedoCmd));
            OnPropertyChanged(nameof(SaveCmd));
            OnPropertyChanged(nameof(BuildCmd));
            OnPropertyChanged(nameof(BuildExeCmd));
            OnPropertyChanged(nameof(UndoSelectionCmd));
            OnPropertyChanged(nameof(RedoSelectionCmd));
            OnPropertyChanged(nameof(DebugStartCmd));
            OnPropertyChanged(nameof(DebugStartWithoutDebuggingCmd));
            OnPropertyChanged(nameof(DebugStopCmd));
        }

        public static async Task<Project> Load(string file)
        {
            Debug.Assert(File.Exists(file));
            var proj = Serializer.FromFile<Project>(file);
            proj.Location = Path.GetDirectoryName(file) + "\\";

            var configName = VisualStudio.GetConfigName(proj.DllBuildConfig);
            var dll = $@"{proj.Location}x64\{configName}\{proj.Name}.dll";

            if (!File.Exists(dll)) await proj.BuildGameDll(false);
            else proj.LoadGameDll();
            proj.SetCommands();

            return proj;
        }

        public static async Task<Project> Load(ProjectData data)
        {
            Debug.Assert(data != null && File.Exists(data.FullPath));
            var proj = Serializer.FromFile<Project>(data.FullPath);
            proj.Location = Path.GetDirectoryName(data.ProjectPath) + "\\";

            var configName = VisualStudio.GetConfigName(proj.DllBuildConfig);
            var dll = $@"{proj.Location}x64\{configName}\{proj.Name}.dll";
            
            if (!File.Exists(dll)) await proj.BuildGameDll(false);
            else proj.LoadGameDll();
            proj.SetCommands();

            return proj;
        }

        public static void Save(Project proj)
        {
            Serializer.ToFile(proj, proj.FullPath);
            Logger.Info($"Saved project {proj.Name} to {proj.FullPath}");
        }

        private void SaveToBinary()
        {
            var bin = $@"{Location}x64\{VisualStudio.GetConfigName(ExeBuildConfig)}\game.bin";

            using var bw = new BinaryWriter(File.Open(bin, FileMode.Create, FileAccess.Write));
            bw.Write(ActiveScene.Entities.Count);
            foreach (var entity in ActiveScene.Entities)
            {
                bw.Write(0); // entity type
                bw.Write(entity.Components.Count);
                foreach (var comp in entity.Components)
                {
                    bw.Write((int)comp.ToEnumType());
                    comp.WriteToBinary(bw);
                }
            }
        }

        public void Unload()
        {
            UnloadGameDll();
            VisualStudio.CloseInstance();
            HistoryManager.Reset();
            SelectionHistoryManager.Reset();
        }

        [OnDeserialized]
        private /*async*/ void OnDeserialized(StreamingContext ctx)
        {
            if (_scenes != null)
            {
                Scenes = new ReadOnlyObservableCollection<Scene>(_scenes);
                OnPropertyChanged(nameof(Scenes));
            }

            ActiveScene = Scenes.FirstOrDefault(x => x.IsActive);

            Debug.Assert(ActiveScene != null);
            
            // var configName = VisualStudio.GetConfigName(DllBuildConfig);
            // var dll = $@"{Location}x64\{configName}\{Name}.dll";
            //
            // if (!File.Exists(dll)) await BuildGameDll(false);
            // else LoadGameDll();
            // SetCommands();
        }

        private async Task BuildGameExe(bool showWindow = true)
        {
            try
            {
                await Task.Run(() => VisualStudio.BuildSolution(this, ExeBuildConfig, showWindow));
                if (VisualStudio.BuildSucceeded)
                {
                    SaveToBinary();
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Error($"Failed to build the game exe");
                throw;
            }
        }

        private async Task BuildGameDll(bool showWindow = true)
        {
            try
            {
                UnloadGameDll();
                await Task.Run(() => VisualStudio.BuildSolution(this, DllBuildConfig, showWindow));
                if (VisualStudio.BuildSucceeded)
                {
                    LoadGameDll();
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Logger.Error($"Failed to build the game dll");
                throw;
            }

        }

        private void LoadGameDll()
        {
            Logger.Info("Loading game dll");

            var configName = VisualStudio.GetConfigName(DllBuildConfig);
            var dll = $@"{Location}x64\{configName}\{Name}.dll";
            Debug.WriteLine($"Dll: {dll} and Location: {Location}");
            AvailableScripts = null;
            if (File.Exists(dll) && EngineAPI.LoadGameDll(dll) != 0)
            {
                AvailableScripts = EngineAPI.GetScriptNames();
                ActiveScene.Entities.Where(x => x.GetComponent<Script>() != null).ToList().ForEach(x => x.IsActive = true);
                Logger.Info("Game dll loaded successfully");
            }
            else
            {
                Logger.Warn("Failed to load the game dll. Try rebuilding the project");
            }
        }

        private void UnloadGameDll()
        {
            Logger.Info("Unloading game dll");
            ActiveScene.Entities.Where(x => x.GetComponent<Script>() != null).ToList().ForEach(x => x.IsActive = false);
            if (EngineAPI.UnloadGameDll() != 0)
            {
                Logger.Info("Game dll unloaded successfully");
                AvailableScripts = null;
            }
        }


        private async Task RunGame(bool debug)
        {
            await Task.Run(() => VisualStudio.BuildSolution(this, ExeBuildConfig, debug));
            if (VisualStudio.BuildSucceeded)
            {
                SaveToBinary();
                await Task.Run(() => VisualStudio.Run(this, ExeBuildConfig, debug));
            }
        }

        private async Task StopGame() => await Task.Run(VisualStudio.Stop);


        private void AddSceneInternal(string sceneName)
        {
            Debug.Assert(!string.IsNullOrEmpty(sceneName.Trim()));
            _scenes.Add(new Scene(this, sceneName));
        }

        private void RemoveSceneInternal(Scene scene)
        {
            Debug.Assert(_scenes.Contains(scene));
            _scenes.Remove(scene);
        }

    }
}
