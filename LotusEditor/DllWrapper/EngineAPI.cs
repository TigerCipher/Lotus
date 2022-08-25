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
        private const string _lotusDll = "LotusDLL.dll";

        [DllImport(_lotusDll, CharSet = CharSet.Ansi)]
        public static extern int LoadGameDll(string dllPath);

        [DllImport(_lotusDll)]
        public static extern int UnloadGameDll();

        internal static class EntityAPI
        {
            [DllImport(_lotusDll)]
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

            [DllImport(_lotusDll)]
            private static extern void RemoveEntity(int id);

            public static void RemoveEntity(Entity entity)
            {
                RemoveEntity(entity.EntityId);
            }
        }

    }
}
