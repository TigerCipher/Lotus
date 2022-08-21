using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using LotusEditor.Components;
using LotusEditor.EngineAPIStructs;

namespace LotusEditor.EngineAPIStructs
{

    [StructLayout(LayoutKind.Sequential)]
    class TransformComponent
    {
        public Vector3 Position;
        public Vector3 Rotation;
        public Vector3 Scale;
    }

    [StructLayout(LayoutKind.Sequential)]
    class EntityDesc
    {
        public TransformComponent Transform = new TransformComponent();
    }
}

namespace LotusEditor.DllWrapper
{
    internal static class EngineAPI
    {
        private const string _dllName = "LotusDLL.dll";

        [DllImport(_dllName)]
        private static extern int CreateEntity(EntityDesc desc);

        public static int CreateEntity(Entity entity)
        {
            EntityDesc desc = new EntityDesc();
            // Transform
            {
                var c = entity.GetComponent<Transform>();
                desc.Transform.Position = c.Position;
                desc.Transform.Rotation = c.Rotation;
                desc.Transform.Scale = c.Scale;
            }

            return CreateEntity(desc);
        }

        [DllImport(_dllName)]
        private static extern void RemoveEntity(int id);

        public static void RemoveEntity(Entity entity)
        {
            RemoveEntity(entity.EntityId);
        }
    }
}
