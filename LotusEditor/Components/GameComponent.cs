using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace LotusEditor.Components
{
    [DataContract]
    internal class GameComponent : ViewModelBase
    {
        [DataMember]
        public GameEntity Owner { get; set; }

        public GameComponent(GameEntity owner)
        {
            Debug.Assert(owner != null);
            Owner = owner;
        }
    }
}
