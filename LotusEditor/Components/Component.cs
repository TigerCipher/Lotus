using System;
using System.Collections.Generic;
using System.Diagnostics;
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
    }


    abstract class MSComponent<T> : ViewModelBase, IMSComponent where T : Component
    {

    }
}
