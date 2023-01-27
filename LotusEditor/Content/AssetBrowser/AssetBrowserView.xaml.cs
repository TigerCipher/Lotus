using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
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
using LotusEditor.GameProject;

namespace LotusEditor.Content
{
    /// <summary>
    /// Interaction logic for AssetBrowserView.xaml
    /// </summary>
    public partial class AssetBrowserView : UserControl
    {
        public AssetBrowserView()
        {
            DataContext = null;
            InitializeComponent();
            Loaded += OnAssetBrowserLoaded;
        }

        private void OnAssetBrowserLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnAssetBrowserLoaded;
            if (Application.Current?.MainWindow != null)
            {
                Application.Current.MainWindow.DataContextChanged += OnProjectChanged;
            }

            OnProjectChanged(null, new DependencyPropertyChangedEventArgs(DataContextProperty, null, Project.Current));
        }

        private void OnProjectChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            (DataContext as AssetBrowser)?.Dispose();
            DataContext = null;
            if (e.NewValue is Project project)
            {
                Debug.Assert(e.NewValue == Project.Current);
                var assetBrowser = new AssetBrowser(project);
                assetBrowser.PropertyChanged += OnSelectedFolderChanged;
                DataContext = assetBrowser;
            }
        }

        private void OnSelectedFolderChanged(object sender, PropertyChangedEventArgs e)
        {
            var vm = sender as AssetBrowser;
            if (e.PropertyName == nameof(vm.SelectedFolder) && !string.IsNullOrEmpty(vm.SelectedFolder))
            {
                // TODO
            }
        }
    }
}
