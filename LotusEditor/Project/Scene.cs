using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace LotusEditor.Project
{
    [DataContract]
    public class Scene : ViewModelBase
    {
        private string _name;
        [DataMember]
        public string Name
        {
            get => _name;
            set
            {
                if (_name == value) return;
                _name = value;
                OnPropertyChanged(nameof(Name));
            }

        }

        [DataMember]
        public Project Project { get; private set; }

        public Scene(Project proj, string name)
        {
            Debug.Assert(proj != null);
            Project = proj;
            Name = name;
        }
    }
}
