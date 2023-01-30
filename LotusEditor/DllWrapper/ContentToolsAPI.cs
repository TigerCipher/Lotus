using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using LotusEditor.ContentToolsAPIStructs;
using LotusEditor.Utility;

namespace LotusEditor.ContentToolsAPIStructs
{
    [StructLayout(LayoutKind.Sequential)]
    class GeometryImportSettings
    {
        public float SmoothingAngle = 178f;
        public byte CalculateNormals = 0;
        public byte CalculateTangents = 1;
        public byte ReverseHandedness = 0;
        public byte ImportEmbededTextures = 1;
        public byte ImportAnimations = 1;

        public void FromContentSettings(Content.Geometry geometry)
        {
            var settings = geometry.ImportSettings;

            SmoothingAngle = settings.SmoothingAngle;
            CalculateNormals = ToByte(settings.CalculateNormals);
            CalculateTangents = ToByte(settings.CalculateTangents);
            ReverseHandedness = ToByte(settings.ReverseHandedness);
            ImportEmbededTextures = ToByte(settings.ImportEmbededTextures);
            ImportAnimations = ToByte(settings.ImportAnimations);
        }

        private byte ToByte(bool val) => val ? (byte)1 : (byte)0;
    }

    [StructLayout(LayoutKind.Sequential)]
    class SceneData : IDisposable
    {
        public IntPtr Data;
        public int DataSize;
        public GeometryImportSettings ImportSettings = new();


        ~SceneData()
        {
            Dispose();
        }

        public void Dispose()
        {
            Marshal.FreeCoTaskMem(Data);
            GC.SuppressFinalize(this);
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    class PrimitiveCreateInfo
    {
        public Content.PrimitiveMeshType Type;
        public int SegmentX;
        public int SegmentY;
        public int SegmentZ;
        public Vector3 Size = new(1f);
        public int LOD = 0;
    }
}

namespace LotusEditor.DllWrapper
{
    static class ContentToolsAPI
    {
        private const string _toolsDLL = "ContentTools.dll";

        [DllImport(_toolsDLL)]
        private static extern void CreatePrimitiveMesh([In, Out] SceneData data, PrimitiveCreateInfo info);

        [DllImport(_toolsDLL)]
        private static extern void ImportFbx(string file, [In, Out] SceneData data);

        private static void GeometryFromSceneData(Content.Geometry geometry, Action<SceneData> sceneDataGenerator,
            string failureMessage)
        {
            Debug.Assert(geometry != null);
            using var sceneData = new SceneData();
            try
            {
                sceneData.ImportSettings.FromContentSettings(geometry);
                sceneDataGenerator(sceneData);
                Debug.Assert(sceneData.Data != IntPtr.Zero && sceneData.DataSize > 0);
                var data = new byte[sceneData.DataSize];
                Marshal.Copy(sceneData.Data, data, 0, sceneData.DataSize);
                geometry.FromRawData(data);
            }
            catch (Exception ex)
            {
                Logger.Error($"{failureMessage} with reason: {ex.Message}");
            }
        }

        public static void CreatePrimitiveMesh(Content.Geometry geometry, PrimitiveCreateInfo info)
        {
            GeometryFromSceneData(geometry, (sceneData) => CreatePrimitiveMesh(sceneData, info), $"Failed to create {info.Type} primitive mesh");
        }


        public static void ImportFbx(string file, Content.Geometry geometry)
        {
            GeometryFromSceneData(geometry, (sceneData) => ImportFbx(file, sceneData), $"Failed to import FBX file: {file}");
        }
    }
}
