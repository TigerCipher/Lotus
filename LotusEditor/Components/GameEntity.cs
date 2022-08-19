using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using LotusEditor.GameProject;
using LotusEditor.Utility;

namespace LotusEditor.Components
{
    [DataContract]
    [KnownType(typeof(Transform))]
    internal class GameEntity : ViewModelBase
    {
        private string _name;
        [DataMember]
        public string Name { get => _name; set { if(_name == value) return; _name = value; OnPropertyChanged(nameof(Name)); } }
        [DataMember]
        public Scene ParentScene { get; private set; }

        private bool _isEnabled = true;
        [DataMember]
        public bool IsEnabled { get => _isEnabled; set { if(_isEnabled == value) return; _isEnabled = value; OnPropertyChanged(nameof(IsEnabled)); } }

        [DataMember(Name = nameof(Components))]
        private readonly ObservableCollection<GameComponent> _components = new();
        public ReadOnlyObservableCollection<GameComponent> Components { get; private set; }

        public ICommand RenameCmd { get; private set; }
        public ICommand EnableCmd { get; private set; }

        public GameEntity(Scene scene)
        {
            Debug.Assert(scene != null);
            ParentScene = scene;
            _components.Add(new Transform(this));

            OnDeserialized(new StreamingContext());
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext ctx)
        {
            if (_components != null)
            {
                Components = new ReadOnlyObservableCollection<GameComponent>(_components);
                OnPropertyChanged(nameof(Components));
            }

            RenameCmd = new RelayCommand<string>(x =>
            {
                var oldName = _name;
                Name = x;
                Project.HistoryManager.AddUndoRedoAction(new UndoRedoAction($"Rename entity '{oldName}' to '{x}'", nameof(Name), this, oldName, x));
            }, x => x != _name);
        }

    }
}
