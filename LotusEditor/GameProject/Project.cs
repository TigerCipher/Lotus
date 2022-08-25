using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using LotusEditor.DllWrapper;
using LotusEditor.GameDev;
using LotusEditor.Utility;

namespace LotusEditor.GameProject
{

    enum BuildConfiguration
    {
        DEBUG,
        DEBUG_DLL,
        RELEASE,
        RELEASE_DLL
    }

    [DataContract(Name = "Game")]
    internal class Project : ViewModelBase
    {
        public static string Extension { get; } = ".lproj";
        [DataMember] public string Name { get; private set; } = "New Project";

        public string Location { get; private set; }
        public string FullPath => $"{Location}{Name}{Extension}";
        public string SolutionName => $@"{Location}{Name}.sln";


        private static readonly string[] _configNames = new[] { "Debug", "DebugDll", "Release", "ReleaseDll" };

        private int _buildConfig;
        [DataMember]
        public int BuildConfig { get => _buildConfig; set { if (_buildConfig == value) return; _buildConfig = value; OnPropertyChanged(nameof(BuildConfig)); } }

        public BuildConfiguration ExeBuildConfig =>
            BuildConfig == 0 ? BuildConfiguration.DEBUG : BuildConfiguration.RELEASE;

        public BuildConfiguration DllBuildConfig =>
            BuildConfig == 0 ? BuildConfiguration.DEBUG_DLL : BuildConfiguration.RELEASE_DLL;

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



        public static Project Current => Application.Current.MainWindow!.DataContext as Project;

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

            UndoSelectionCmd = new RelayCommand<object>(x => SelectionHistoryManager.Undo(), x => SelectionHistoryManager.UndoList.Any());
            RedoSelectionCmd = new RelayCommand<object>(x => SelectionHistoryManager.Redo(), x => SelectionHistoryManager.RedoList.Any());

            OnPropertyChanged(nameof(AddSceneCmd));
            OnPropertyChanged(nameof(RemoveSceneCmd));
            OnPropertyChanged(nameof(UndoCmd));
            OnPropertyChanged(nameof(RedoCmd));
            OnPropertyChanged(nameof(SaveCmd));
            OnPropertyChanged(nameof(BuildCmd));
            OnPropertyChanged(nameof(UndoSelectionCmd));
            OnPropertyChanged(nameof(RedoSelectionCmd));
        }

        public static Project Load(string file)
        {
            Debug.Assert(File.Exists(file));
            return Serializer.FromFile<Project>(file);
        }

        public static Project Load(ProjectData data)
        {
            Debug.Assert(data != null && File.Exists(data.FullPath));
            var proj = Serializer.FromFile<Project>(data.FullPath);
            proj.Location = Path.GetDirectoryName(data.ProjectPath) + "\\";

            return proj;
        }

        public static void Save(Project proj)
        {
            Serializer.ToFile(proj, proj.FullPath);
            Logger.Info($"Saved project {proj.Name} to {proj.FullPath}");
        }

        public void Unload()
        {
            VisualStudio.CloseInstance();
            HistoryManager.Reset();
            SelectionHistoryManager.Reset();
        }

        [OnDeserialized]
        private async void OnDeserialized(StreamingContext ctx)
        {
            if (_scenes != null)
            {
                Scenes = new ReadOnlyObservableCollection<Scene>(_scenes);
                OnPropertyChanged(nameof(Scenes));
            }

            ActiveScene = Scenes.FirstOrDefault(x => x.IsActive);

            await BuildGameDll(false);

            SetCommands();
        }

        private async Task BuildGameDll(bool showWindow = true)
        {
            try
            {
                UnloadGameDll();

                await Task.Run(() => VisualStudio.BuildSolution(this, GetConfigName(DllBuildConfig), showWindow));
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

            var configName = GetConfigName(DllBuildConfig);
            var dll = $@"{Location}x64\{configName}\{Name}.dll";
            if (File.Exists(dll) && EngineAPI.LoadGameDll(dll) != 0)
            {
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
            if (EngineAPI.UnloadGameDll() != 0)
            {
                Logger.Info("Game dll unloaded successfully");
            }
        }

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


        private static string GetConfigName(BuildConfiguration config) => _configNames[(int)config];

    }
}
