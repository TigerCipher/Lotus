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
using LotusEditor.DllWrapper;
using LotusEditor.GameProject;
using LotusEditor.Utility;

namespace LotusEditor.Content
{
    enum PrimitiveMeshType
    {
        Plane, Cube, UvSphere, IcoSphere, Cylinder, Capsule
    }

    enum ElementsType
    {
        Position = 0x00,
        Normals = 0x01,
        TSpace = 0x03,
        Joints = 0x04,
        Colors = 0x08,
    }

    enum PrimitiveTopology
    {
        PointList = 1,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
    }

    class Mesh : ViewModelBase
    {

        public static int PositionSize = sizeof(float) * 3;

        private int _elementSize;
        public int ElementSize { get => _elementSize; set { if(_elementSize == value) return; _elementSize = value; OnPropertyChanged(nameof(ElementSize)); } }

        private int _vertexCount;
        public int VertexCount { get => _vertexCount; set { if (_vertexCount == value) return; _vertexCount = value; OnPropertyChanged(nameof(VertexCount)); } }

        private int _indexSize;
        public int IndexSize { get => _indexSize; set { if (_indexSize == value) return; _indexSize = value; OnPropertyChanged(nameof(IndexSize)); } }

        private int _indexCount;
        public int IndexCount { get => _indexCount; set { if (_indexCount == value) return; _indexCount = value; OnPropertyChanged(nameof(IndexCount)); } }

        private string _name;
        public string Name { get => _name; set { if (_name == value) return; _name = value; OnPropertyChanged(nameof(Name)); } }

        public ElementsType ElementsType { get; set; }
        public PrimitiveTopology PrimitiveTopology { get; set; }

        public byte[] Positions { get; set; }
        public byte[] Elements { get; set; }
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

        public void FromBinary(BinaryReader reader)
        {
            CalculateNormals = reader.ReadBoolean();
            CalculateTangents = reader.ReadBoolean();
            SmoothingAngle = reader.ReadSingle();
            ReverseHandedness = reader.ReadBoolean();
            ImportEmbededTextures = reader.ReadBoolean();
            ImportAnimations = reader.ReadBoolean();
        }
    }

    internal class Geometry : Asset
    {

        private readonly object _lock = new();

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

            var mesh = new Mesh() { Name = meshName };
            var lodId = reader.ReadInt32();
            mesh.ElementSize = reader.ReadInt32();
            mesh.ElementsType = (ElementsType)reader.ReadInt32();
            mesh.PrimitiveTopology = PrimitiveTopology.TriangleList; // Tools currently only supports triangle list
            mesh.VertexCount = reader.ReadInt32();
            mesh.IndexSize = reader.ReadInt32();
            mesh.IndexCount = reader.ReadInt32();
            var lodThreshold = reader.ReadSingle();

            var elementBufferSize = mesh.ElementSize * mesh.VertexCount;
            var indexBufferSize = mesh.IndexSize    * mesh.IndexCount;
            mesh.Positions = reader.ReadBytes(Mesh.PositionSize * mesh.VertexCount);
            mesh.Elements = reader.ReadBytes(elementBufferSize);
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
            return lodGroup < _lodGroups.Count ? _lodGroups[lodGroup] : null;
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
                    var meshName = ContentUtil.FixFilename(
                        path + fileName + ((_lodGroups.Count > 1) ? "_" + ((lodGroup.LODS.Count > 1) ? lodGroup.Name : lodGroup.LODS[0].Name) : string.Empty)) + AssetFileExtension;
                    Guid = TryGetAssetInfo(meshName) is AssetInfo info && info.Type == Type ? info.Guid : Guid.NewGuid();
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

        // struct {
        //      u32 lod_count,
        //      struct {
        //          f32 lod_threshold,
        //          u32 submesh_count,
        //          u32 size_of_submeshes,
        //          struct {
        //              u32 element_size, u32 vertex_count,
        //              u32 index_count, u32 elements_type, u32 primitive_topology,
        //              u8 positions[sizeof(f32) * 3 * vertex_count],
        //              u8 elements[sizeof(element_size) * vertex_count],
        //              u8 indices[index_size * index_count]
        //          } submeshes[submesh_count]
        //      } mesh_lods[lod_count]
        // } geometry
        public override byte[] PackForEngine()
        {
            using var writer = new BinaryWriter(new MemoryStream());

            writer.Write(GetLodGroup().LODS.Count);
            foreach (var lod in GetLodGroup().LODS)
            {
                writer.Write(lod.LodThreshold);
                writer.Write(lod.Meshes.Count);
                var sizeOfSubmeshesPosition = writer.BaseStream.Position;
                writer.Write(0);
                foreach (var mesh in lod.Meshes)
                {
                    writer.Write(mesh.ElementSize);
                    writer.Write(mesh.VertexCount);
                    writer.Write(mesh.IndexCount);
                    writer.Write((int)mesh.ElementsType);
                    writer.Write((int)mesh.PrimitiveTopology);

                    var alignedPos = new byte[MathHelper.AlignSizeUp(mesh.Positions.Length, 4)];
                    Array.Copy(mesh.Positions, alignedPos, mesh.Positions.Length);

                    var alignedElem = new byte[MathHelper.AlignSizeUp(mesh.Elements.Length, 4)];
                    Array.Copy(mesh.Elements, alignedElem, mesh.Elements.Length);

                    writer.Write(alignedPos);
                    writer.Write(alignedElem);
                    writer.Write(mesh.Indices);
                }

                var endOfSubmeshes = writer.BaseStream.Position;
                var sizeOfSubmeshes = (int)(endOfSubmeshes - sizeOfSubmeshesPosition - sizeof(int));

                writer.BaseStream.Position = sizeOfSubmeshesPosition;
                writer.Write(sizeOfSubmeshes);
                writer.BaseStream.Position = endOfSubmeshes;

            }

            writer.Flush();

            var data = (writer.BaseStream as MemoryStream)?.ToArray();
            Debug.Assert(data?.Length > 0);

            // Testing purposes only

            using (var fs = new FileStream(@"..\..\Tests\model.model", FileMode.Create))
            {
                fs.Write(data, 0, data.Length);
            }

            //////////////////

            return data;
        }

        public override void Import(string file)
        {
            Debug.Assert(File.Exists(file));
            Debug.Assert(!string.IsNullOrEmpty(FullPath));

            var ext = Path.GetExtension(file).ToLower();

            SourcePath = file;

            try
            {
                if (ext == ".fbx")
                {
                    ImportFbx(file);
                }
            }
            catch (Exception ex)
            {
                Logger.Error($"Failed to read {file} for import. Reason: {ex.Message}");
            }
        }

        public override void Load(string file)
        {
            Debug.Assert(File.Exists(file));
            Debug.Assert(Path.GetExtension(file).ToLower() == AssetFileExtension);

            try
            {
                byte[] data = null;
                using (var reader = new BinaryReader(File.Open(file, FileMode.Open, FileAccess.Read)))
                {
                    ReadAssetFileHeader(reader);
                    ImportSettings.FromBinary(reader);
                    int dataLen = reader.ReadInt32();
                    Debug.Assert(dataLen > 0);
                    data = reader.ReadBytes(dataLen);
                }

                Debug.Assert(data.Length > 0);

                using (var reader = new BinaryReader(new MemoryStream(data)))
                {
                    LODGroup lodgroup = new LODGroup();
                    lodgroup.Name = reader.ReadString();
                    var lodCount = reader.ReadInt32();

                    for (var i = 0; i < lodCount; ++i)
                    {
                        lodgroup.LODS.Add(BinaryToLOD(reader));
                    }

                    _lodGroups.Clear();
                    _lodGroups.Add(lodgroup);
                }

                // Testing

                 PackForEngine();


                //////////////////
            }
            catch (Exception ex)
            {
                Logger.Error($"Failed to load geometry asset from file: {file}");
                Logger.Error($"Encountered exception: {ex.Message}");
            }
        }

        private void ImportFbx(string file)
        {
            Logger.Info($"Importing FBX file {file} (This may take a minute, especially if in a debug environment)");
            var temp = Application.Current.Dispatcher.Invoke(() => Project.Current.TempFolder);
            if (string.IsNullOrEmpty(temp)) return;

            lock (_lock)
            {
                if (!Directory.Exists(temp)) Directory.CreateDirectory(temp);
            }

            var tempFile = $"{temp}{ContentUtil.GetRandomString()}.fbx";
            File.Copy(file, tempFile, true);
            ContentToolsAPI.ImportFbx(tempFile, this);
        }

        private byte[] GenerateIcon(MeshLOD lod)
        {
            Logger.Info("Generating asset icon");
            var width = AssetFileInfo.IconWidth * 4; // width pixels * 4, so 4x sampling

            using var memStream = new MemoryStream();
            BitmapSource bmp = null;

            Application.Current.Dispatcher.Invoke(() =>
            {
                bmp = Editors.GeometryEditor.GeometryView.RenderToBitmap(new Editors.GeometryEditor.MeshRenderer(lod, null),
                    width, width);
                bmp = new TransformedBitmap(bmp, new ScaleTransform(0.25, 0.25, 0.5, 0.5));
                memStream.SetLength(0);

                Logger.Info("Encoding icon");
                var encoder = new PngBitmapEncoder();
                encoder.Frames.Add(BitmapFrame.Create(bmp));
                encoder.Save(memStream);
            });


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
                writer.Write(mesh.Name);
                writer.Write(mesh.ElementSize);
                writer.Write((int)mesh.ElementsType);
                writer.Write((int)mesh.PrimitiveTopology);
                writer.Write(mesh.VertexCount);
                writer.Write(mesh.IndexSize);
                writer.Write(mesh.IndexCount);
                writer.Write(mesh.Positions);
                writer.Write(mesh.Elements);
                writer.Write(mesh.Indices);
            }

            var meshDataSize = writer.BaseStream.Position - meshBegin;
            Debug.Assert(meshDataSize > 0);

            var buffer = (writer.BaseStream as MemoryStream)?.ToArray();
            hash = ContentUtil.ComputeHash(buffer, (int)meshBegin, (int)meshDataSize);
        }

        private MeshLOD BinaryToLOD(BinaryReader reader)
        {
            var lod = new MeshLOD();
            lod.Name = reader.ReadString();
            lod.LodThreshold = reader.ReadSingle();
            var meshCount = reader.ReadInt32();
            for (var i = 0; i < meshCount; ++i)
            {
                var mesh = new Mesh()
                {
                    Name = reader.ReadString(),
                    ElementSize = reader.ReadInt32(),
                    ElementsType = (ElementsType)reader.ReadInt32(),
                    PrimitiveTopology = (PrimitiveTopology)reader.ReadInt32(),
                    VertexCount = reader.ReadInt32(),
                    IndexSize = reader.ReadInt32(),
                    IndexCount = reader.ReadInt32()
                };
                mesh.Positions = reader.ReadBytes(Mesh.PositionSize * mesh.VertexCount);
                mesh.Elements = reader.ReadBytes(mesh.ElementSize * mesh.VertexCount);
                mesh.Indices = reader.ReadBytes(mesh.IndexSize * mesh.IndexCount);

                lod.Meshes.Add(mesh);
            }

            return lod;
        }
    }
}
