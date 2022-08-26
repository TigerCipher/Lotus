using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace LotusEditor.Components
{
    [DataContract]
    internal class Script : Component
    {

        private string _name;
        [DataMember]
        public string Name { get => _name; set { if(_name == value) return; _name = value; OnPropertyChanged(nameof(Name)); } }

        public Script(Entity owner) : base(owner)
        {
        }

        public override IMSComponent GetMSComponent(MSEntity msEnt) => new MSScript(msEnt);
        public override void WriteToBinary(BinaryWriter bw)
        {
            var nameBytes = Encoding.UTF8.GetBytes(Name);
            bw.Write(nameBytes.Length);
            bw.Write(nameBytes);
        }
    }

    sealed class MSScript : MSComponent<Script>
    {
        private string _name;
        public string Name { get => _name; set { if (_name == value) return; _name = value; OnPropertyChanged(nameof(Name)); } }

        public MSScript(MSEntity msEnt) : base(msEnt)
        {
            Refresh();
        }

        protected override bool UpdateComponents(string propertyName)
        {
            if (propertyName != nameof(Name)) return false;

            SelectedComponents.ForEach(c => c.Name = _name);
            return true;

        }

        protected override bool UpdateMSComponent()
        {
            Name = MSEntity.GetMixedValue(SelectedComponents, x => x.Name);
            return true;
        }
    }
}
