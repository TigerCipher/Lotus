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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace LotusEditor.Editors.GeometryEditor
{
    /// <summary>
    /// Interaction logic for GeometryDetailView.xaml
    /// </summary>
    public partial class GeometryDetailView : UserControl
    {
        public GeometryDetailView()
        {
            InitializeComponent();
        }

        private void OnCheckboxClick_Highlight(object sender, RoutedEventArgs e)
        {
            var vm = DataContext as GeometryEditor;
            foreach (var mesh in vm.MeshRenderer.Meshes)
            {
                mesh.IsHighlighted = false;
            }

            var checkbox = sender as CheckBox;
            (checkbox.DataContext as MeshRendererVertexData).IsHighlighted = checkbox.IsChecked == true;
        }

        private void OnCheckboxClick_Isolate(object sender, RoutedEventArgs e)
        {
            var vm = DataContext as GeometryEditor;
            foreach (var mesh in vm.MeshRenderer.Meshes)
            {
                mesh.IsIsolated = false;
            }

            var checkbox = sender as CheckBox;
            var m = checkbox.DataContext as MeshRendererVertexData;
            m.IsIsolated = checkbox.IsChecked == true;

            if (Tag is GeometryView geometryView)
            {
                geometryView.SetGeometry(m.IsIsolated ? vm.MeshRenderer.Meshes.IndexOf(m) : -1);
            }
        }
    }
}
