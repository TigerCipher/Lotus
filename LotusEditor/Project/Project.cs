using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Dynamic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace LotusEditor.Project
{
    [DataContract(Name = "Game")]
    public class Project : ViewModelBase
    {
        public static string Extension { get; } = ".lproj";

        [DataMember]
        public string Name { get; private set; }

        [DataMember]
        public string Location { get; private set; }


        public string FullPath => $"{Location}{Name}{Extension}";

        [DataMember(Name = "Scenes")]
        private ObservableCollection<Scene> _scenes = new ObservableCollection<Scene>();

        public ReadOnlyObservableCollection<Scene> Scenes { get; }


        public Project(string name, string path)
        {
            Name = name;
            Location = path;

            _scenes.Add(new Scene(this, "Default Scene"));
        }
    }
}
