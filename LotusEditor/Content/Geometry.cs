using System;
using System.CodeDom;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using LotusEditor.Utility;

namespace LotusEditor.Content
{
    enum PrimitiveMeshType
    {
        Plane, Cube, UvSphere, IcoSphere, Cylinder, Capsule
    }

    class Mesh : ViewModelBase
    {
        private int _vertexSize;
        public int VertexSize { get => _vertexSize; set { if (_vertexSize == value) return; _vertexSize = value; OnPropertyChanged(nameof(VertexSize)); } }

        private int _vertexCount;
        public int VertexCount { get => _vertexCount; set { if (_vertexCount == value) return; _vertexCount = value; OnPropertyChanged(nameof(VertexCount)); } }

        private int _indexSize;
        public int IndexSize { get => _indexSize; set { if (_indexSize == value) return; _indexSize = value; OnPropertyChanged(nameof(IndexSize)); } }

        private int _indexCount;
        public int IndexCount { get => _indexCount; set { if (_indexCount == value) return; _indexCount = value; OnPropertyChanged(nameof(IndexCount)); } }

        public byte[] Vertices { get; set; }
        public byte[] Indices { get; set; }
    }

    class MeshLOD : ViewModelBase
    {
        private string _name;
        public string Name { get => _name; set { if (_name == value) return; _name = value; OnPropertyChanged(nameof(Name)); } }

        private float _lodThreshold;
        public float LodThreshold { get => _lodThreshold; set { if (_lodThreshold == value) return; _lodThreshold = value; OnPropertyChanged(nameof(LodThreshold)); } }

        public ObservableCollection<Mesh> Meshes { get; } = new();
    }

    class LODGroup : ViewModelBase
    {
        private string _name;
        public string Name { get => _name; set { if (_name == value) return; _name = value; OnPropertyChanged(nameof(Name)); } }

        public ObservableCollection<MeshLOD> LODS { get; } = new();
    }

    class GeometryImportSettings : ViewModelBase
    {
        private float _smoothingAngle;
        public float SmoothingAngle { get => _smoothingAngle; set { if (_smoothingAngle == value) return; _smoothingAngle = value; OnPropertyChanged(nameof(SmoothingAngle)); } }

        private bool _calculateNormals;
        public bool CalculateNormals { get => _calculateNormals; set { if (_calculateNormals == value) return; _calculateNormals = value; OnPropertyChanged(nameof(CalculateNormals)); } }

        private bool _calculateTangents;
        public bool CalculateTangents { get => _calculateTangents; set { if (_calculateTangents == value) return; _calculateTangents = value; OnPropertyChanged(nameof(CalculateTangents)); } }

        private bool _reverseHandedness;
        public bool ReverseHandedness { get => _reverseHandedness; set { if (_reverseHandedness == value) return; _reverseHandedness = value; OnPropertyChanged(nameof(ReverseHandedness)); } }

        private bool _importEmbededTextures;
        public bool ImportEmbededTextures { get => _importEmbededTextures; set { if (_importEmbededTextures == value) return; _importEmbededTextures = value; OnPropertyChanged(nameof(ImportEmbededTextures)); } }

        private bool _importAnimations;
        public bool ImportAnimations { get => _importAnimations; set { if (_importAnimations == value) return; _importAnimations = value; OnPropertyChanged(nameof(ImportAnimations)); } }

        public GeometryImportSettings()
        {
            SmoothingAngle = 178f;
            CalculateNormals = false;
            CalculateTangents = false;
            ReverseHandedness = false;
            ImportEmbededTextures = true;
            ImportAnimations = true;
        }

        public void ToBinary(BinaryWriter writer)
        {
            writer.Write(CalculateNormals);
            writer.Write(CalculateTangents);
            writer.Write(SmoothingAngle);
            writer.Write(ReverseHandedness);
            writer.Write(ImportEmbededTextures);
            writer.Write(ImportAnimations);
        }
    }

    internal class Geometry : Asset
    {

        private readonly List<LODGroup> _lodGroups = new();

        public GeometryImportSettings ImportSettings { get; } = new();

        public Geometry() : base(AssetType.Mesh)
        {
        }

        public void FromRawData(byte[] data)
        {
            Debug.Assert(data?.Length > 0);
            _lodGroups.Clear();

            using var reader = new BinaryReader(new MemoryStream(data));
            // skip scene name for now
            var s = reader.ReadInt32();
            reader.BaseStream.Position += s;

            // lods
            var numLodGroups = reader.ReadInt32();
            Debug.Assert(numLodGroups > 0);
            for (var i = 0; i < numLodGroups; ++i)
            {
                s = reader.ReadInt32();
                string lodGroupName;
                if (s > 0)
                {
                    var nameBytes = reader.ReadBytes(s);
                    lodGroupName = Encoding.UTF8.GetString(nameBytes);
                }
                else
                {
                    lodGroupName = $"lod_{ContentUtil.GetRandomString()}";
                }

                // meshes
                var numMeshes = reader.ReadInt32();
                Debug.Assert(numMeshes > 0);
                var lods = ReadMeshLODs(numMeshes, reader);
                var lodGroup = new LODGroup() { Name = lodGroupName };
                lods.ForEach(l => lodGroup.LODS.Add(l));
                _lodGroups.Add(lodGroup);
            }
        }

        private static List<MeshLOD> ReadMeshLODs(int numMeshes, BinaryReader reader)
        {
            var lodIds = new List<int>();
            var lodList = new List<MeshLOD>();
            for (var i = 0; i < numMeshes; i++)
            {
                ReadMeshes(reader, lodIds, lodList);
            }

            return lodList;
        }

        private static void ReadMeshes(BinaryReader reader, List<int> lodIds, List<MeshLOD> lodList)
        {
            var s = reader.ReadInt32();
            string meshName;
            if (s > 0)
            {
                var nameBytes = reader.ReadBytes(s);
                meshName = Encoding.UTF8.GetString(nameBytes);
            }
            else
            {
                meshName = $"mesh_{ContentUtil.GetRandomString()}";
            }

            var mesh = new Mesh();
            var lodId = reader.ReadInt32();
            mesh.VertexSize = reader.ReadInt32();
            mesh.VertexCount = reader.ReadInt32();
            mesh.IndexSize = reader.ReadInt32();
            mesh.IndexCount = reader.ReadInt32();
            var lodThreshold = reader.ReadSingle();

            var vertexBufferSize = mesh.VertexSize * mesh.VertexCount;
            var indexBufferSize = mesh.IndexSize * mesh.IndexCount;
            mesh.Vertices = reader.ReadBytes(vertexBufferSize);
            mesh.Indices = reader.ReadBytes(indexBufferSize);

            MeshLOD lod;
            if (ID.IsValid(lodId) && lodIds.Contains(lodId))
            {
                lod = lodList[lodIds.IndexOf(lodId)];
                Debug.Assert(lod != null);
            }
            else
            {
                lodIds.Add(lodId);
                lod = new MeshLOD() { Name = meshName, LodThreshold = lodThreshold };
                lodList.Add(lod);
            }

            lod.Meshes.Add(mesh);
        }

        public LODGroup GetLodGroup(int lodGroup = 0)
        {
            Debug.Assert(lodGroup >= 0 && lodGroup < _lodGroups.Count);
            return _lodGroups.Any() ? _lodGroups[lodGroup] : null;
        }

        public override IEnumerable<string> Save(string file)
        {
            Debug.Assert(_lodGroups.Any());
            var savedFiles = new List<string>();
            if (!_lodGroups.Any()) return savedFiles;
            var path = Path.GetDirectoryName(file) + Path.DirectorySeparatorChar;
            var fileName = Path.GetFileNameWithoutExtension(file);

            try
            {
                foreach (var lodGroup in _lodGroups)
                {
                    Debug.Assert(lodGroup.LODS.Any());
                    // use name of highest detail mesh
                    var meshName = ContentUtil.FixFilename(_lodGroups.Count > 1 ?
                        path + fileName + "_" + lodGroup.LODS[0].Name + AssetFileExtension :
                        path + fileName + AssetFileExtension);
                    Guid = Guid.NewGuid(); // need dif id for each asset file
                    Logger.Info($"Saving mesh with highest lod with name {lodGroup.LODS[0].Name} and guid {Guid.ToString()}");
                    byte[] data = null;
                    using (var writer = new BinaryWriter(new MemoryStream()))
                    {
                        writer.Write(lodGroup.Name);
                        writer.Write(lodGroup.LODS.Count);
                        var hashes = new List<byte>();
                        Logger.Info("Saving LODs");
                        foreach (var lod in lodGroup.LODS)
                        {
                            LodToBinary(lod, writer, out var hash);
                            hashes.AddRange(hash);
                        }

                        Logger.Info("Computing hash");
                        Hash = ContentUtil.ComputeHash(hashes.ToArray());
                        data = (writer.BaseStream as MemoryStream)?.ToArray();
                        Icon = GenerateIcon(lodGroup.LODS[0]);
                    }

                    Debug.Assert(data?.Length > 0);

                    using (var writer = new BinaryWriter(File.Open(meshName, FileMode.Create, FileAccess.Write)))
                    {
                        WriteAssetFileHeader(writer);
                        Logger.Info("Saving import settings");
                        ImportSettings.ToBinary(writer);
                        Logger.Info("Saving data buffer");
                        writer.Write(data.Length);
                        writer.Write(data);
                    }

                    savedFiles.Add(meshName);
                    Logger.Info($"Geometry asset [{meshName}] successfully saved!");
                }
            }
            catch (Exception e)
            {
                Debug.Write(e.Message);
                Logger.Error($"Failed to save geometry asset to {fileName}");
            }

            return savedFiles;
        }

        private byte[] GenerateIcon(MeshLOD lod)
        {
            Logger.Info("Generating asset icon");
            const int width = 90 * 4; // 90 pixels * 4, so 4x sampling
            BitmapSource bmp = null;

            Application.Current.Dispatcher.Invoke(() =>
            {
                bmp = Editors.GeometryEditor.GeometryView.RenderToBitmap(new Editors.GeometryEditor.MeshRenderer(lod, null),
                    width, width);
                bmp = new TransformedBitmap(bmp, new ScaleTransform(0.25, 0.25, 0.5, 0.5));
            });

            using var memStream = new MemoryStream();
            memStream.SetLength(0);

            Logger.Info("Encoding icon");
            var encoder = new PngBitmapEncoder();
            encoder.Frames.Add(BitmapFrame.Create(bmp));
            encoder.Save(memStream);

            Logger.Info("Icon created");
            return memStream.ToArray();
        }


        private void LodToBinary(MeshLOD lod, BinaryWriter writer, out byte[] hash)
        {
            writer.Write(lod.Name);
            writer.Write(lod.LodThreshold);
            writer.Write(lod.Meshes.Count);

            var meshBegin = writer.BaseStream.Position;

            foreach (var mesh in lod.Meshes)
            {
                writer.Write(mesh.VertexSize);
                writer.Write(mesh.VertexCount);
                writer.Write(mesh.IndexCount);
                writer.Write(mesh.IndexCount);
                writer.Write(mesh.Vertices);
                writer.Write(mesh.Indices);
            }

            var meshDataSize = writer.BaseStream.Position - meshBegin;
            Debug.Assert(meshDataSize > 0);

            var buffer = (writer.BaseStream as MemoryStream).ToArray();
            hash = ContentUtil.ComputeHash(buffer, (int)meshBegin, (int)meshDataSize);
        }
    }
}
