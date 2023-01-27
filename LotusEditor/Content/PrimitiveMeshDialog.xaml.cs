using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using LotusEditor.ContentToolsAPIStructs;
using LotusEditor.DllWrapper;
using LotusEditor.Editors;
using LotusEditor.Editors.GeometryEditor;
using LotusEditor.GameProject;
using LotusEditor.Utility;
using LotusEditor.Utility.Controls;
using Microsoft.Win32;
using Window = System.Windows.Window;

namespace LotusEditor.Content
{
    /// <summary>
    /// Interaction logic for PrimitiveMeshDialog.xaml
    /// </summary>
    public partial class PrimitiveMeshDialog : Window
    {

        private static readonly List<ImageBrush> _textures = new();

        public PrimitiveMeshDialog()
        {
            InitializeComponent();
            Loaded += (s, e) => UpdatePrimitive();
        }

        static PrimitiveMeshDialog()
        {
            LoadTextures();
        }

        private static void LoadTextures()
        {
            var uris = new List<Uri>
            {
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/PlaneTexture.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/PlaneTexture.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/Checkermap.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/PlaneTexture.png"),
                new Uri("pack://application:,,,/Resources/PrimitiveMeshView/PlaneTexture.png"),
            };

            _textures.Clear();
            foreach (var uri in uris)
            {
                var res = Application.GetResourceStream(uri);
                using var reader = new BinaryReader(res.Stream);
                var data = reader.ReadBytes((int)res.Stream.Length);
                var imageRes = (BitmapSource)new ImageSourceConverter().ConvertFrom(data);
                imageRes?.Freeze();
                var brush = new ImageBrush(imageRes)
                {
                    Transform = new ScaleTransform(1, -1, 0.5, 0.5),
                    ViewportUnits = BrushMappingMode.Absolute
                };
                brush.Freeze();
                _textures.Add(brush);
            }
        }

        private void OnPrimitiveType_ComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e) =>
            UpdatePrimitive();

        private void OnSlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e) =>
            UpdatePrimitive();

        private void OnScalarBox_ValueChanged(object sender, RoutedEventArgs e) =>
            UpdatePrimitive();

        private float Value(ScalarBox scalarBox, float min)
        {
            float.TryParse(scalarBox.Value, out var result);
            return Math.Max(result, min);
        }

        private void UpdatePrimitive()
        {
            if (!IsInitialized) return;
            var primitiveType = (PrimitiveMeshType)primitiveTypeComboBox.SelectedItem;
            var info = new PrimitiveCreateInfo() { Type = primitiveType };
            var smoothingAngle = 0;
            switch (primitiveType)
            {
                case PrimitiveMeshType.Plane:
                {
                    info.SegmentX = (int)xSliderPlane.Value;
                    info.SegmentZ = (int)zSliderPlane.Value;
                    info.Size.X = Value(widthScalarBoxPlane, 0.001f);
                    info.Size.Z = Value(lengthScalarBoxPlane, 0.001f);
                    break;
                }
                case PrimitiveMeshType.Cube:
                    return;
                case PrimitiveMeshType.UvSphere:
                {
                    info.SegmentX = (int)xSliderUvSphere.Value;
                    info.SegmentY = (int)ySliderUvSphere.Value;
                    info.Size.X = Value(xScalarBoxUvSphere, 0.001f);
                    info.Size.Y = Value(yScalarBoxUvSphere, 0.001f);
                    info.Size.Z = Value(zScalarBoxUvSphere, 0.001f);
                    smoothingAngle = (int)angleSliderUvSphere.Value;
                    break;
                }
                case PrimitiveMeshType.IcoSphere:
                    return;
                case PrimitiveMeshType.Cylinder:
                    return;
                case PrimitiveMeshType.Capsule:
                    return;
            }

            var geometry = new Geometry();
            geometry.ImportSettings.SmoothingAngle = smoothingAngle;
            ContentToolsAPI.CreatePrimitiveMesh(geometry, info);
            (DataContext as GeometryEditor).SetAsset(geometry);
            TexturesCheckBox_OnClick(texturesCheckBox, null);
        }


        private void TexturesCheckBox_OnClick(object sender, RoutedEventArgs e)
        {
            Brush brush = Brushes.White;
            if ((sender as CheckBox).IsChecked == true)
            {
                brush = _textures[(int)primitiveTypeComboBox.SelectedItem];
            }

            var vm = DataContext as GeometryEditor;
            foreach (var mesh in vm.MeshRenderer.Meshes)
            {
                mesh.Diffuse = brush;
            }
        }

        private void SaveButton_OnClick(object sender, RoutedEventArgs e)
        {
            var dlg = new SaveFileDialog()
            {
                InitialDirectory = Project.Current.ContentPath,
                Filter = "Asset File (*.asset)|*.asset"
            };

            if (dlg.ShowDialog() == true)
            {
                Debug.Assert(!string.IsNullOrEmpty(dlg.FileName));
                // Logger.Info($"Saving asset file to {dlg.FileName}");
                var asset = (DataContext as IAssetEditor).Asset;
                Debug.Assert(asset != null);
                asset.Save(dlg.FileName);
            }
        }
    }
}
