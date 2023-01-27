
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;

using System.Windows.Input;

using LotusEditor.GameProject;

namespace LotusEditor.Content
{
    /// <summary>
    /// Interaction logic for AssetBrowserView.xaml
    /// </summary>
    public partial class AssetBrowserView : UserControl
    {
        private string _sortedProperty = nameof(AssetFileInfo.FileName);
        private ListSortDirection _sortDirection;

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

            folderListView.AddHandler(Thumb.DragDeltaEvent, new DragDeltaEventHandler(Thumb_DragDelta), true);

            folderListView.Items.SortDescriptions.Add(new SortDescription(_sortedProperty, _sortDirection));
            
        }

        private void Thumb_DragDelta(object sender, DragDeltaEventArgs e)
        {
            if (e.OriginalSource is Thumb { TemplatedParent: GridViewColumnHeader header })
            {
                if (header.Column.ActualWidth < 50)
                {
                    header.Column.Width = 50;
                }else if (header.Column.ActualWidth > 250)
                {
                    header.Column.Width = 250;
                }
            }

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
                GeneratePathStackButtons();
            }
        }

        private void GeneratePathStackButtons()
        {
            var vm = DataContext as AssetBrowser;
            var path = Directory.GetParent(Path.TrimEndingDirectorySeparator(vm.SelectedFolder)).FullName;
            var contentPath = Path.TrimEndingDirectorySeparator(vm.AssetsFolder);

            pathStack.Children.RemoveRange(1, pathStack.Children.Count - 1);
            if (vm.SelectedFolder == vm.AssetsFolder) return;
            var paths = new string[3];
            var labels = new string[3];

            int i;
            for (i = 0; i < 3; ++i)
            {
                paths[i] = path;
                labels[i] = path[(path.LastIndexOf(Path.DirectorySeparatorChar) + 1)..];
                if (path == contentPath) break;
                path = path.Substring(0, path.LastIndexOf(Path.DirectorySeparatorChar));
            }

            if (i == 3) i = 2;
            for (; i >= 0; --i)
            {
                var btn = new Button()
                {
                    DataContext = paths[i],
                    Content = new TextBlock() { Text = labels[i], TextTrimming = TextTrimming.CharacterEllipsis }
                };
                pathStack.Children.Add(btn);
                if (i > 0) pathStack.Children.Add(new System.Windows.Shapes.Path());
            }
        }

        private void OnGridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            var column = sender as GridViewColumnHeader;
            var sortBy = column.Tag.ToString();

            folderListView.Items.SortDescriptions.Clear();
            var newDir = ListSortDirection.Ascending;

            if (_sortedProperty == sortBy && _sortDirection == newDir)
            {
                newDir = ListSortDirection.Descending;
            }

            _sortDirection = newDir;
            _sortedProperty = sortBy;

            folderListView.Items.SortDescriptions.Add(new SortDescription(sortBy, newDir));
            
        }

        private void OnContent_Item_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            var info = (sender as FrameworkElement)?.DataContext as AssetFileInfo;
            ExecuteSelection(info);
        }

        private void ExecuteSelection(AssetFileInfo info)
        {
            if (info == null) return;

            if (info.IsDirectory)
            {
                var vm = DataContext as AssetBrowser;
                vm.SelectedFolder = info.FullPath;
            }
        }

        private void OnContent_Item_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                var info = (sender as FrameworkElement)?.DataContext as AssetFileInfo;
                ExecuteSelection(info);
            }
        }

        private void OnPathStack_Button_Click(object sender, RoutedEventArgs e)
        {
            var vm = DataContext as AssetBrowser;
            vm.SelectedFolder = (sender as Button)?.DataContext as string;
        }
    }
}
