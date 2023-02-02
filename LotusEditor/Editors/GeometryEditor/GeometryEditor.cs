using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.IO.MemoryMappedFiles;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using LotusEditor.Content;
using LotusEditor.Utility;

namespace LotusEditor.Editors.GeometryEditor
{

    // temp until I implement a proper renderer
    class MeshRendererVertexData : ViewModelBase
    {

        private Brush _diffuse = Brushes.White;
        public Brush Diffuse { get => IsHighlighted ? Brushes.Orange : _diffuse; set { if (_diffuse == value) return; _diffuse = value; OnPropertyChanged(nameof(Diffuse)); } }

        private Brush _specular = new SolidColorBrush((Color)ColorConverter.ConvertFromString("#ff111111"));
        public Brush Specular { get => _specular; set { if (_specular == value) return; _specular = value; OnPropertyChanged(nameof(Specular)); } }

        private bool _isHighlighted;

        public bool IsHighlighted
        {
            get => _isHighlighted;
            set
            {
                if (_isHighlighted == value) return;
                _isHighlighted = value;
                OnPropertyChanged(nameof(IsHighlighted));
                OnPropertyChanged(nameof(Diffuse));
            }
        }

        private bool _isIsolated;
        public bool IsIsolated { get => _isIsolated; set { if (_isIsolated == value) return; _isIsolated = value; OnPropertyChanged(nameof(IsIsolated)); } }


        public Point3DCollection Positions { get; } = new();
        public Vector3DCollection Normals { get; } = new();
        public PointCollection UVs { get; } = new();
        public Int32Collection Indices { get; } = new();
        public string Name { get; set; }
    }

    // also temp
    class MeshRenderer : ViewModelBase
    {
        public ObservableCollection<MeshRendererVertexData> Meshes { get; } = new();

        private Vector3D _cameraDir = new(0, 0, -10);
        public Vector3D CameraDirection { get => _cameraDir; set { if (_cameraDir == value) return; _cameraDir = value; OnPropertyChanged(nameof(CameraDirection)); } }

        private Point3D _cameraPos = new(0, 0, 10);
        public Point3D CameraPosition
        {
            get => _cameraPos; set
            {
                if (_cameraPos == value) return; _cameraPos = value;
                CameraDirection = new Vector3D(-value.X, -value.Y, -value.Z); OnPropertyChanged(nameof(OffsetCameraPosition)); OnPropertyChanged(nameof(CameraPosition));
            }
        }

        private Point3D _cameraTarget = new(0, 0, 0);
        public Point3D CameraTarget { get => _cameraTarget; set { if (_cameraTarget == value) return; _cameraTarget = value; OnPropertyChanged(nameof(OffsetCameraPosition)); OnPropertyChanged(nameof(CameraTarget)); } }

        public Point3D OffsetCameraPosition => new(CameraPosition.X + CameraTarget.X, CameraPosition.Y + CameraTarget.Y, CameraPosition.Z + CameraTarget.Z);

        private Color _keyLight = (Color)ColorConverter.ConvertFromString("#ffaeaeae");
        public Color KeyLight { get => _keyLight; set { if (_keyLight == value) return; _keyLight = value; OnPropertyChanged(nameof(KeyLight)); } }

        private Color _skyLight = (Color)ColorConverter.ConvertFromString("#ff111b30");
        public Color SkyLight { get => _skyLight; set { if (_skyLight == value) return; _skyLight = value; OnPropertyChanged(nameof(SkyLight)); } }

        private Color _groundLight = (Color)ColorConverter.ConvertFromString("#ff3f2f1e");
        public Color GroundLight { get => _groundLight; set { if (_groundLight == value) return; _groundLight = value; OnPropertyChanged(nameof(GroundLight)); } }

        private Color _ambientLight = (Color)ColorConverter.ConvertFromString("#ff3b3b3b");
        public Color AmbientLight { get => _ambientLight; set { if (_ambientLight == value) return; _ambientLight = value; OnPropertyChanged(nameof(AmbientLight)); } }

        public MeshRenderer(MeshLOD lod, MeshRenderer old)
        {
            Debug.Assert(lod?.Meshes.Any() == true);

            double minX, minY, minZ;
            minX = minY = minZ = double.MaxValue;
            double maxX, maxY, maxZ;
            maxX = maxY = maxZ = double.MinValue;
            Vector3D avgNormal = new();
            // Unpack the normals
            var intervals = 2.0f / ((1 << 16) - 1);
            foreach (var mesh in lod.Meshes)
            {
                var vertexData = new MeshRendererVertexData() { Name = mesh.Name };
                using (var reader = new BinaryReader(new MemoryStream(mesh.Positions)))
                    for (int i = 0; i < mesh.VertexCount; ++i)
                    {
                        var posX = reader.ReadSingle();
                        var posY = reader.ReadSingle();
                        var posZ = reader.ReadSingle();
                        
                        vertexData.Positions.Add(new Point3D(posX, posY, posZ));

                        minX = Math.Min(minX, posX);
                        maxX = Math.Max(maxX, posX);
                        minY = Math.Min(minY, posY);
                        maxY = Math.Max(maxY, posY);
                        minZ = Math.Min(minZ, posZ);
                        maxZ = Math.Max(maxZ, posZ);
                    }
                // Read normals
                if (mesh.ElementsType.HasFlag(ElementsType.Normals))
                {
                    var tSpaceOffset = 0;
                    if (mesh.ElementsType.HasFlag(ElementsType.Joints)) tSpaceOffset = sizeof(short) * 4;

                    // Read tangent space
                    using (var reader = new BinaryReader(new MemoryStream(mesh.Elements)))
                        for (int i = 0; i < mesh.VertexCount; i++)
                        {
                            var signs = (reader.ReadUInt32() >> 24) & 0x000000ff;
                            reader.BaseStream.Position += tSpaceOffset;
                            var normX = reader.ReadUInt16() * intervals - 1.0f;
                            var normY = reader.ReadUInt16() * intervals - 1.0f;
                            var normZ = Math.Sqrt(Math.Clamp(1f - (normX * normX + normY * normY), 0f, 1f)) *
                                        ((signs & 0x2) - 1f);
                            var normal = new Vector3D(normX, normY, normZ);
                            normal.Normalize();
                            vertexData.Normals.Add(normal);
                            avgNormal += normal;

                            // Read UVs
                            if (mesh.ElementsType.HasFlag(ElementsType.TSpace))
                            {
                                reader.BaseStream.Position += (sizeof(short) * 2); // skip over tangents
                                var u = reader.ReadSingle();
                                var v = reader.ReadSingle();
                                vertexData.UVs.Add(new Point(u, v));
                            }

                            if (mesh.ElementsType.HasFlag(ElementsType.Joints) &&
                                mesh.ElementsType.HasFlag(ElementsType.Colors))
                            {
                                reader.BaseStream.Position += 4;
                            }
                        }
                }


                using (var reader = new BinaryReader(new MemoryStream(mesh.Indices)))
                    if (mesh.IndexSize == sizeof(short))
                    {
                        for (var i = 0; i < mesh.IndexCount; ++i)
                        {
                            vertexData.Indices.Add(reader.ReadUInt16());
                        }
                    }
                    else
                    {
                        for (var i = 0; i < mesh.IndexCount; ++i)
                        {
                            vertexData.Indices.Add(reader.ReadInt32());
                        }
                    }

                vertexData.Positions.Freeze();
                vertexData.Normals.Freeze();
                vertexData.UVs.Freeze();
                vertexData.Indices.Freeze();
                Meshes.Add(vertexData);
            }

            if (old != null)
            {
                CameraTarget = old.CameraTarget;
                CameraPosition = old.CameraPosition;

                foreach (var mesh in old.Meshes)
                {
                    mesh.IsHighlighted = false;
                }

                foreach (var mesh in Meshes)
                {
                    mesh.Diffuse = old.Meshes.First().Diffuse;
                }
            }
            else
            {
                var width = maxX - minX;
                var height = maxY - minY;
                var depth = maxZ - minZ;
                var radius = new Vector3D(height, width, depth).Length * 1.2;
                if (avgNormal.Length > 0.8)
                {
                    avgNormal.Normalize();
                    avgNormal *= radius;
                    CameraPosition = new Point3D(avgNormal.X, avgNormal.Y, avgNormal.Z);
                }
                else
                {
                    CameraPosition = new Point3D(width, height * 0.5, radius);
                }

                CameraTarget = new Point3D(minX + width * 0.5, minY + height * 0.5, minZ + depth * 0.5);
            }
        }
    }

    internal class GeometryEditor : ViewModelBase, IAssetEditor
    {
        Asset IAssetEditor.Asset => Geometry;

        private Content.Geometry _geometry;
        public Content.Geometry Geometry { get => _geometry; set { if (_geometry == value) return; _geometry = value; OnPropertyChanged(nameof(Geometry)); } }

        private MeshRenderer _meshRenderer;

        public MeshRenderer MeshRenderer
        {
            get => _meshRenderer;
            set
            {
                if (_meshRenderer == value) return;
                _meshRenderer = value;
                OnPropertyChanged(nameof(MeshRenderer));
                var lods = Geometry.GetLodGroup().LODS;
                MaxLODIndex = (lods.Count > 0) ? lods.Count - 1 : 0;
                OnPropertyChanged(nameof(MaxLODIndex));
                if (lods.Count > 1)
                {
                    MeshRenderer.PropertyChanged += (s, e) =>
                    {
                        if (e.PropertyName == nameof(MeshRenderer.OffsetCameraPosition) && AutoLOD) ComputeLOD(lods);
                    };
                    ComputeLOD(lods);
                }
            }
        }



        private bool _autoLOD = true;
        public bool AutoLOD { get => _autoLOD; set { if (_autoLOD == value) return; _autoLOD = value; OnPropertyChanged(nameof(AutoLOD)); } }

        public int MaxLODIndex { get; private set; }

        private int _lodIndex;

        public int LodIndex
        {
            get => _lodIndex;
            set
            {
                var lods = Geometry.GetLodGroup().LODS;
                value = Math.Clamp(value, 0, lods.Count - 1);
                if (_lodIndex == value) return;
                _lodIndex = value;
                OnPropertyChanged(nameof(LodIndex));
                MeshRenderer = new MeshRenderer(lods[value], MeshRenderer);
            }
        }

        public void SetAsset(Content.Asset asset)
        {
            Debug.Assert(asset is Content.Geometry);
            if (asset is Content.Geometry geometry)
            {
                Geometry = geometry;
                var numLods = geometry.GetLodGroup().LODS.Count;
                if (LodIndex >= numLods)
                {
                    LodIndex = numLods - 1;
                }
                else
                {
                    MeshRenderer = new MeshRenderer(Geometry.GetLodGroup().LODS[0], MeshRenderer);
                }
            }
        }

        public async void SetAsset(AssetInfo info)
        {
            try
            {
                Debug.Assert(info != null && File.Exists(info.FullPath));
                var geometry = new Content.Geometry();
                await Task.Run(() =>
                {
                    geometry.Load(info.FullPath);
                });

                SetAsset(geometry);
            }
            catch (Exception ex)
            {
                Logger.Warn($"Exception occurred while loading asset: {ex.Message}");
            }
        }



        private void ComputeLOD(IList<MeshLOD> lods)
        {
            if (!AutoLOD) return;

            var p = MeshRenderer.OffsetCameraPosition;
            var dist = new Vector3D(p.X, p.Y, p.Z).Length;
            for (var i = MaxLODIndex; i >= 0; --i)
            {
                if (lods[i].LodThreshold < dist)
                {
                    LodIndex = i;
                    break;
                }
            }
        }
    }
}
