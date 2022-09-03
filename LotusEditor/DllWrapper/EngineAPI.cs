using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using LotusEditor.Components;
using LotusEditor.EngineAPIStructs;
using LotusEditor.GameProject;
using LotusEditor.Utility;

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
    class ScriptComponent
    {
        public IntPtr ScriptCreator;
    }

    [StructLayout(LayoutKind.Sequential)]
    class EntityDesc
    {
        public TransformComponent Transform = new();
        public ScriptComponent Script = new();
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

        [DllImport(_lotusDll)]
        public static extern IntPtr GetScriptCreator(string name);

        [DllImport(_lotusDll)]
        [return: MarshalAs(UnmanagedType.SafeArray)]
        public static extern string[] GetScriptNames();

        [DllImport(_lotusDll)]
        public static extern int CreateRenderSurface(IntPtr host, int width, int height);

        [DllImport(_lotusDll)]
        public static extern void RemoveRenderSurface(int surfaceId);

        [DllImport(_lotusDll)]
        public static extern IntPtr GetWindowHandle(int surfaceId);

        [DllImport(_lotusDll)]
        public static extern int ResizeRenderSurface(int surfaceId);

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

                // Script
                {
                    var c = entity.GetComponent<Script>();
                    if (c != null && Project.Current != null)
                    {
                        if (Project.Current.AvailableScripts.Contains(c.Name))
                        {
                            desc.Script.ScriptCreator = GetScriptCreator(c.Name);
                        }
                        else
                        {
                            Logger.Error($"Failed to find script: {c.Name}. Entity will be created without it");
                        }
                    }
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
