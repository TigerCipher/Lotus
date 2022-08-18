using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Windows;
using LotusEditor.Utility;

namespace LotusEditor.GameProject
{
    [DataContract(Name = "Game")]
    public class Project : ViewModelBase
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
        }

        public void Unload()
        {

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
        }


        public void AddScene(string sceneName)
        {
            Debug.Assert(!string.IsNullOrEmpty(sceneName.Trim()));
            _scenes.Add(new Scene(this, sceneName)); 
        }

        public void RemoveScene(Scene scene)
        {
            Debug.Assert(_scenes.Contains(scene));
            _scenes.Remove(scene);
        }

    }
}
