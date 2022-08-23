using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using LotusEditor.Utility;

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

        public override IMSComponent GetMSComponent(MSEntity msEnt) => new MSTransform(msEnt);
    }

    sealed class MSTransform : MSComponent<Transform>
    {

        private float? _posX;
        public float? PosX { get => _posX; set { if(_posX.IsEqual(value)) return; _posX = value; OnPropertyChanged(nameof(PosX)); } }

        private float? _posY;
        public float? PosY { get => _posY; set { if(_posY.IsEqual(value)) return; _posY = value; OnPropertyChanged(nameof(PosY)); } }

        private float? _posZ;
        public float? PosZ { get => _posZ; set { if(_posZ.IsEqual(value)) return; _posZ = value; OnPropertyChanged(nameof(PosZ)); } }

        private float? _rotX;
        public float? RotX { get => _rotX; set { if(_rotX.IsEqual(value)) return; _rotX = value; OnPropertyChanged(nameof(RotX)); } }

        private float? _rotY;
        public float? RotY { get => _rotY; set { if(_rotY.IsEqual(value)) return; _rotY = value; OnPropertyChanged(nameof(RotY)); } }

        private float? _rotZ;
        public float? RotZ { get => _rotZ; set { if(_rotZ.IsEqual(value)) return; _rotZ = value; OnPropertyChanged(nameof(RotZ)); } }

        private float? _scaleX;
        public float? ScaleX { get => _scaleX; set { if (_scaleX.IsEqual(value)) return; _scaleX = value; OnPropertyChanged(nameof(ScaleX)); } }

        private float? _scaleY;
        public float? ScaleY { get => _scaleY; set { if (_scaleY.IsEqual(value)) return; _scaleY = value; OnPropertyChanged(nameof(ScaleY)); } }

        private float? _scaleZ;
        public float? ScaleZ { get => _scaleZ; set { if (_scaleZ.IsEqual(value)) return; _scaleZ = value; OnPropertyChanged(nameof(ScaleZ)); } }

        public MSTransform(MSEntity msEnt) : base(msEnt)
        {
            Refresh();
        }

        protected override bool UpdateComponents(string propertyName)
        {
            switch (propertyName)
            {
                case nameof(PosX):
                case nameof(PosZ):
                case nameof(PosY):
                    SelectedComponents.ForEach(c =>
                    {
                        Debug.WriteLine($"Old PosX: {c.Position.X}");
                        c.Position = new Vector3(_posX ?? c.Position.X, _posY ?? c.Position.Y,
                                _posZ ?? c.Position.Z);
                        Debug.WriteLine($"New PosX: {c.Position.X}");
                    });
                    return true;

                case nameof(RotX):
                case nameof(RotZ):
                case nameof(RotY):
                    SelectedComponents.ForEach(c => c.Rotation = new Vector3(_rotX ?? c.Rotation.X, _rotY ?? c.Rotation.Y, _rotZ ?? c.Rotation.Z));
                    return true;

                case nameof(ScaleX):
                case nameof(ScaleZ):
                case nameof(ScaleY):
                    SelectedComponents.ForEach(c => c.Scale = new Vector3(_scaleX ?? c.Scale.X, _scaleY ?? c.Scale.Y, _scaleZ ?? c.Scale.Z));
                    return true;
            }

            return false;
        }

        protected override bool UpdateMSComponent()
        {
            Debug.WriteLine("Updating ms comp");
            PosX = MSEntity.GetMixedValue(SelectedComponents, x => x.Position.X);
            PosY = MSEntity.GetMixedValue(SelectedComponents, x => x.Position.Y);
            PosZ = MSEntity.GetMixedValue(SelectedComponents, x => x.Position.Z);

            RotX = MSEntity.GetMixedValue(SelectedComponents, x => x.Rotation.X);
            RotY = MSEntity.GetMixedValue(SelectedComponents, x => x.Rotation.Y);
            RotZ = MSEntity.GetMixedValue(SelectedComponents, x => x.Rotation.Z);

            ScaleX = MSEntity.GetMixedValue(SelectedComponents, x => x.Scale.X);
            ScaleY = MSEntity.GetMixedValue(SelectedComponents, x => x.Scale.Y);
            ScaleZ = MSEntity.GetMixedValue(SelectedComponents, x => x.Scale.Z);

            return true;
        }
    }
}
