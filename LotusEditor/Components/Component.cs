using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace LotusEditor.Components
{

    interface IMSComponent
    {
        
    }

    [DataContract]
    internal abstract class Component : ViewModelBase
    {
        [DataMember]
        public Entity Owner { get; set; }

        public Component(Entity owner)
        {
            Debug.Assert(owner != null);
            Owner = owner;
        }

        public abstract IMSComponent GetMSComponent(MSEntity msEnt);
        public abstract void WriteToBinary(BinaryWriter bw);
    }


    abstract class MSComponent<T> : ViewModelBase, IMSComponent where T : Component
    {
        private bool _enableUpdates = true;
        public List<T> SelectedComponents { get; }

        public MSComponent(MSEntity msEnt)
        {
            Debug.Assert(msEnt?.SelectedEntities?.Any() == true);
            SelectedComponents = msEnt.SelectedEntities.Select(ent => ent.GetComponent<T>()).ToList();
            PropertyChanged += (s, e) =>
            {
                if (_enableUpdates) UpdateComponents(e.PropertyName);
            };
        }

        protected abstract bool UpdateComponents(string propertyName);
        protected abstract bool UpdateMSComponent();

        public void Refresh()
        {
            _enableUpdates = false;
            UpdateMSComponent();
            _enableUpdates = true;
        }
    }
}
