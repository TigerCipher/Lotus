using System;
using System.Collections.Generic;
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
using System.Windows.Media.Media3D;
using System.Windows.Navigation;
using System.Windows.Shapes;
using EnvDTE;

namespace LotusEditor.Editors.GeometryEditor
{
    /// <summary>
    /// Interaction logic for GeometryView.xaml
    /// </summary>
    public partial class GeometryView : UserControl
    {
        private Point _clickPosition;
        private bool _capturedLeft;
        private bool _capturedRight;

        public GeometryView()
        {
            InitializeComponent();
            DataContextChanged += (s, e) => SetGeometry();
        }

        private void SetGeometry(int index = -1)
        {
            if (DataContext is not MeshRenderer vm) return;
            if (vm.Meshes.Any() && viewport.Children.Count == 2)
            {
                viewport.Children.RemoveAt(1);
            }

            var meshIndex = 0;
            var modelGroup = new Model3DGroup();
            foreach (var mesh in vm.Meshes)
            {
                if (index != -1 && meshIndex != index)
                {
                    ++meshIndex;
                    continue;
                }

                var mesh3d = new MeshGeometry3D()
                {
                    Positions = mesh.Positions,
                    Normals = mesh.Normals,
                    TriangleIndices = mesh.Indices,
                    TextureCoordinates = mesh.UVs
                };

                var diffuse = new DiffuseMaterial(mesh.Diffuse);
                var specular = new SpecularMaterial(mesh.Specular, 50);
                var matGroup = new MaterialGroup();
                matGroup.Children.Add(diffuse);
                matGroup.Children.Add(specular);

                var model = new GeometryModel3D(mesh3d, matGroup);
                modelGroup.Children.Add(model);

                var binding = new Binding(nameof(mesh.Diffuse)) { Source = mesh };
                BindingOperations.SetBinding(diffuse, DiffuseMaterial.BrushProperty, binding);

                if (meshIndex == index) break;
            }

            var visual = new ModelVisual3D() { Content = modelGroup };
            viewport.Children.Add(visual);
        }

        private void OnGrid_MouseButtonLeftDown(object sender, MouseButtonEventArgs e)
        {
            _clickPosition = e.GetPosition(this);
            _capturedLeft = true;

            Mouse.Capture(sender as UIElement);
        }

        private void OnGrid_MouseMove(object sender, MouseEventArgs e)
        {
            if (!_capturedLeft && !_capturedRight) return;
            var pos = e.GetPosition(this);
            var d = pos - _clickPosition;
            if (_capturedLeft && !_capturedRight)
            {
                MoveCamera(d.X, d.Y, 0);
            }
            else if (_capturedRight && !_capturedLeft)
            {
                var vm = DataContext as MeshRenderer;
                var camPos = vm.CameraPosition;
                var yOffset = d.Y * 0.001 * Math.Sqrt(camPos.X * camPos.X + camPos.Z * camPos.Z);
                vm.CameraTarget = new Point3D(vm.CameraTarget.X, vm.CameraTarget.Y + yOffset, vm.CameraTarget.Z);
            }

            _clickPosition = pos;
        }

        private void OnGrid_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            _capturedLeft = false;
            if (!_capturedRight) Mouse.Capture(null);
        }

        private void OnGrid_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            MoveCamera(0, 0, Math.Sign(e.Delta));
        }

        private void OnGrid_MouseButtonRightDown(object sender, MouseButtonEventArgs e)
        {
            _capturedRight = true;
            _clickPosition = e.GetPosition(this);
            Mouse.Capture(sender as UIElement);
        }

        private void OnGrid_MouseButtonRightUp(object sender, MouseButtonEventArgs e)
        {
            _capturedRight = false;
            if (!_capturedLeft) Mouse.Capture(null);
        }

        private void MoveCamera(double dx, double dy, int dz)
        {
            var vm = DataContext as MeshRenderer;
            var v = new Vector3D(vm.CameraPosition.X, vm.CameraPosition.Y, vm.CameraPosition.Z);

            var r = v.Length;
            var theta = Math.Acos(v.Y / r); // We are using right handed coords, so to convert to spherical coords I'm replacing Z with Y, and Y with -Z
            var phi = Math.Atan2(-v.Z, v.X);

            theta -= dy * 0.01;
            phi -= dx * 0.01;
            r *= 1.0 - 0.1 * dz;

            theta = Math.Clamp(theta, 0.0001, Math.PI - 0.0001);
            v.X = r * Math.Sin(theta) * Math.Cos(phi);
            v.Z = -r * Math.Sin(theta) * Math.Sin(phi);
            v.Y = r * Math.Cos(theta);

            vm.CameraPosition = new Point3D(v.X, v.Y, v.Z);
        }
    }
}
