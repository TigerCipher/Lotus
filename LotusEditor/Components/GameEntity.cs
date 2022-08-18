﻿using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using LotusEditor.GameProject;

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

        [DataMember(Name = nameof(Components))]
        private readonly ObservableCollection<GameComponent> _components = new();
        public ReadOnlyObservableCollection<GameComponent> Components { get; private set; }

        public GameEntity(Scene scene)
        {
            Debug.Assert(scene != null);
            ParentScene = scene;
            _components.Add(new Transform(this));
        }

        [OnDeserialized]
        private void OnDeserialized(StreamingContext ctx)
        {
            if (_components != null)
            {
                Components = new ReadOnlyObservableCollection<GameComponent>(_components);
                OnPropertyChanged(nameof(Components));
            }
        }

    }
}
