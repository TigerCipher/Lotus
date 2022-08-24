using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Windows;
using System.Windows.Input;
using LotusEditor.Utility;

namespace LotusEditor.GameProject
{
    [DataContract(Name = "Game")]
    internal class Project : ViewModelBase
    {
        public static string Extension { get; } = ".lproj";
        [DataMember] public string Name { get; private set; } = "New Project";

        public string Location { get; private set; }
        public string FullPath => $"{Location}{Name}{Extension}";
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

        public Project(string name, string path)
        {
            Name = name;
            Location = path;

            OnDeserialized(new StreamingContext());
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
            proj.Location = data.ProjectPath;

            return proj;
        }

        public static void Save(Project proj)
        {
            Serializer.ToFile(proj, proj.FullPath);
            Logger.Info($"Saved project {proj.Name} to {proj.FullPath}");
        }

        public void Unload()
        {
            HistoryManager.Reset();
            SelectionHistoryManager.Reset();
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext ctx)
        {
            if (_scenes != null)
            {
                Scenes = new ReadOnlyObservableCollection<Scene>(_scenes);
                OnPropertyChanged(nameof(Scenes));
            }

            ActiveScene = Scenes.FirstOrDefault(x => x.IsActive);

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

            UndoCmd = new RelayCommand<object>(x => HistoryManager.Undo());
            RedoCmd = new RelayCommand<object>(x => HistoryManager.Redo());
            SaveCmd = new RelayCommand<object>(x => Save(this));

            UndoSelectionCmd = new RelayCommand<object>(x => SelectionHistoryManager.Undo());
            RedoSelectionCmd = new RelayCommand<object>(x => SelectionHistoryManager.Redo());
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

    }
}
