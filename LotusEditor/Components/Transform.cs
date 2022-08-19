using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace LotusEditor.Components
{
    [DataContract]
    internal class Transform : Component
    {
        private Vector3 _position;
        [DataMember]
        public Vector3 Position { get => _position; set { if(_position == value) return; _position = value; OnPropertyChanged(nameof(Position)); } }

        private Vector3 _rotation;
        [DataMember]
        public Vector3 Rotation { get => _rotation; set { if(_rotation == value) return; _rotation = value; OnPropertyChanged(nameof(Rotation)); } }

        private Vector3 _scale;
        [DataMember]
        public Vector3 Scale { get => _scale; set { if(_scale == value) return; _scale = value; OnPropertyChanged(nameof(Scale)); } }

        public Transform(Entity owner) : base(owner)
        {
        }
    }
}
