using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LotusEditor.Components
{

    enum ComponentType
    {
        TRANSFORM,
        SCRIPT,
    }
    static class ComponentFactory
    {

        private static readonly Func<Entity, object, Component>[] _func = new Func<Entity, object, Component>[]
        {
            (entity, data) => new Transform(entity),
            (entity, data) => new Script(entity) {Name = (string)data},
        };

        public static Func<Entity, object, Component> GetCreateFunction(ComponentType compType)
        {
            Debug.Assert((int)compType < _func.Length);
            return _func[(int)compType];
        }

        public static ComponentType ToEnumType(this Component comp)
        {
            return comp switch
            {
                Transform => ComponentType.TRANSFORM,
                Script => ComponentType.SCRIPT,
                _ => throw new ArgumentException("Unknown component type")
            };
        }
    }
}
