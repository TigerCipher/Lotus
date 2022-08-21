using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using LotusEditor;
using LotusEditor.Components;
using LotusEditor.DllWrapper;
using LotusEditor.GameProject;
using LotusEditor.Utility;

namespace LotusEditor.Components
{
    [DataContract]
    [KnownType(typeof(Transform))]
    internal class Entity : ViewModelBase
    {
        private int _entityId = ID.INVALID_ID;
        public int EntityId { get => _entityId; set { if (_entityId == value) return; _entityId = value; OnPropertyChanged(nameof(EntityId)); } }

        private bool _isActive;
        public bool IsActive
        {
            get => _isActive; set
            {
                if (_isActive == value) return; _isActive = value;
                if (_isActive)
                {
                    EntityId = EngineAPI.CreateEntity(this);
                    Debug.Assert(ID.IsValid(_entityId));
                }
                else
                {
                    EngineAPI.RemoveEntity(this);
                }

                OnPropertyChanged(nameof(IsActive));
            }
        }

        private string _name;
        [DataMember]
        public string Name { get => _name; set { if (_name == value) return; _name = value; OnPropertyChanged(nameof(Name)); } }
        [DataMember]
        public Scene ParentScene { get; private set; }

        private bool _isEnabled = true;
        [DataMember]
        public bool IsEnabled { get => _isEnabled; set { if (_isEnabled == value) return; _isEnabled = value; OnPropertyChanged(nameof(IsEnabled)); } }

        [DataMember(Name = nameof(Components))]
        private readonly ObservableCollection<Component> _components = new();
        public ReadOnlyObservableCollection<Component> Components { get; private set; }

        public Entity(Scene scene)
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
                Components = new ReadOnlyObservableCollection<Component>(_components);
                OnPropertyChanged(nameof(Components));
            }

        }

        public Component GetComponent(Type type) => Components.FirstOrDefault(c => c.GetType() == type);

        public T GetComponent<T>() where T : Component => GetComponent(typeof(T)) as T;

    }







    // Multiselection

    abstract class MSEntity : ViewModelBase
    {
        private bool? _isEnabled;
        public bool? IsEnabled { get => _isEnabled; set { if (_isEnabled == value) return; _isEnabled = value; OnPropertyChanged(nameof(IsEnabled)); } }

        private string _name;
        public string Name { get => _name; set { if (_name == value) return; _name = value; OnPropertyChanged(nameof(Name)); } }

        private readonly ObservableCollection<IMSComponent> _components = new();
        public ReadOnlyObservableCollection<IMSComponent> Components { get; }

        public List<Entity> SelectedEntities { get; }


        private bool _enableUpdates = true;

        public MSEntity(List<Entity> entities)
        {
            Debug.Assert(entities?.Any() == true);
            Components = new ReadOnlyObservableCollection<IMSComponent>(_components);
            SelectedEntities = entities;
            PropertyChanged += (s, e) =>
            {
                if (_enableUpdates)
                    UpdateEntities(e.PropertyName);
            };
        }

        protected virtual bool UpdateEntities(string propName)
        {
            switch (propName)
            {
                case nameof(IsEnabled): SelectedEntities.ForEach(x => x.IsEnabled = IsEnabled.Value); return true;
                case nameof(Name): SelectedEntities.ForEach(x => x.Name = Name); return true;
            }

            return false;
        }

        public void Refresh()
        {
            _enableUpdates = false;
            UpdateMSGameEntity();
            _enableUpdates = true;
        }

        protected virtual bool UpdateMSGameEntity()
        {
            IsEnabled = GetMixedValue(SelectedEntities, new Func<Entity, bool>(x => x.IsEnabled));
            Name = GetMixedValue(SelectedEntities, new Func<Entity, string>(x => x.Name));

            return true;
        }

        public static float? GetMixedValue(List<Entity> entities, Func<Entity, float> getProperty)
        {
            var value = getProperty(entities.First());
            foreach (var entity in entities.Skip(1))
            {
                if (!value.IsEqual(getProperty(entity))) return null;
            }
            return value;
        }

        public static bool? GetMixedValue(List<Entity> entities, Func<Entity, bool> getProperty)
        {
            var value = getProperty(entities.First());
            foreach (var entity in entities.Skip(1))
            {
                if (value != getProperty(entity)) return null;
            }
            return value;
        }

        public static string GetMixedValue(List<Entity> entities, Func<Entity, string> getProperty)
        {
            var value = getProperty(entities.First());
            foreach (var entity in entities.Skip(1))
            {
                if (value != getProperty(entity)) return null;
            }
            return value;
        }
    }

    class MSGameEntity : MSEntity
    {
        public MSGameEntity(List<Entity> entities) : base(entities)
        {
            Refresh();
        }
    }
}
