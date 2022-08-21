using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Runtime.Serialization;
using System.Windows.Input;
using LotusEditor.Components;
using LotusEditor.Utility;

namespace LotusEditor.GameProject
{
    [DataContract]
    internal class Scene : ViewModelBase
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

        private bool _isActive;
        [DataMember]
        public bool IsActive
        {
            get => _isActive;
            set
            {
                if (_isActive == value) return;
                _isActive = value;
                OnPropertyChanged(nameof(IsActive));
            }
        }

        [DataMember(Name = nameof(Entities))]
        private readonly ObservableCollection<Entity> _entities = new();
        public ReadOnlyObservableCollection<Entity> Entities { get; private set; }

        public ICommand AddEntityCmd { get; private set; }
        public ICommand RemoveEntityCmd { get; private set; }

        public Scene(Project proj, string name)
        {
            Debug.Assert(proj != null);
            Project = proj;
            Name = name;

            OnDeserialized(new StreamingContext());
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext ctx)
        {
            if (_entities != null)
            {
                Entities = new ReadOnlyObservableCollection<Entity>(_entities);
                OnPropertyChanged(nameof(Entities));
            }

            foreach (var entity in _entities)
            {
                entity.IsActive = IsActive;
            }

            AddEntityCmd = new RelayCommand<Entity>(x =>
            {
                AddEntity(x);
                var index = _entities.Count - 1;
                Project.HistoryManager.AddUndoRedoAction(new UndoRedoAction(
                    $"Add Entity {x.Name} to Scene {Name}",
                    () => RemoveEntity(x),
                    () => AddEntity(x, index)));
            });

            RemoveEntityCmd = new RelayCommand<Entity>(x =>
            {
                var index = _entities.IndexOf(x);
                RemoveEntity(x);

                Project.HistoryManager.AddUndoRedoAction(new UndoRedoAction(
                    $"Remove Entity {x.Name} from Scene {Name}",
                    () => AddEntity(x, index),
                    () => RemoveEntity(x)));
            });

        }

        private void AddEntity(Entity entity, int index = -1)
        {
            Debug.Assert(!_entities.Contains(entity));
            entity.IsActive = IsActive;
            Logger.Info("Adding entity with id " + entity.EntityId);
            if (index == -1)
                _entities.Add(entity);
            else
            {
                _entities.Insert(index, entity);
            }
        }

        private void RemoveEntity(Entity entity)
        {
            Debug.Assert(_entities.Contains(entity));
            entity.IsActive = false;
            Logger.Info("Removing entity with id " + entity.EntityId);
            _entities.Remove(entity);
        }
    }
}
