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

        [DataMember(Name = nameof(GameEntities))]
        private readonly ObservableCollection<GameEntity> _gameEntities = new();
        public ReadOnlyObservableCollection<GameEntity> GameEntities { get; private set; }

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
            if (_gameEntities != null)
            {
                GameEntities = new ReadOnlyObservableCollection<GameEntity>(_gameEntities);
                OnPropertyChanged(nameof(GameEntities));
            }

            AddEntityCmd = new RelayCommand<GameEntity>(x =>
            {
                AddEntity(x);
                var index = _gameEntities.Count - 1;
                Project.HistoryManager.AddUndoRedoAction(new UndoRedoAction(
                    $"Add Entity {x.Name} to Scene {Name}",
                    () => RemoveEntity(x),
                    () => _gameEntities.Insert(index, x)));
            });

            RemoveEntityCmd = new RelayCommand<GameEntity>(x =>
            {
                var index = _gameEntities.IndexOf(x);
                RemoveEntity(x);

                Project.HistoryManager.AddUndoRedoAction(new UndoRedoAction(
                    $"Remove Entity {x.Name} from Scene {Name}",
                    () => _gameEntities.Insert(index, x),
                    () => RemoveEntity(x)));
            });

        }

        private void AddEntity(GameEntity entity)
        {
            Debug.Assert(!_gameEntities.Contains(entity));
            _gameEntities.Add(entity);
        }

        private void RemoveEntity(GameEntity entity)
        {
            Debug.Assert(_gameEntities.Contains(entity));
            _gameEntities.Remove(entity);
        }
    }
}
