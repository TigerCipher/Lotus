
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Input;

using LotusEditor.GameProject;

namespace LotusEditor.Content
{

    // Much of this due to JLRishe from a StackOverflow post back in 2013
    class DataSizeToStringConverter : IValueConverter
    {
        static readonly string[] _sizeSuffixes =
                   { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };

        static string SizeSuffix(long value, int decimalPlaces = 1)
        {
            if (value <= 0 || decimalPlaces < 0) return string.Empty;

            // mag is 0 for bytes, 1 for KB, 2, for MB, etc.
            var mag = (int)Math.Log(value, 1024);

            // 1L << (mag * 10) == 2 ^ (10 * mag) 
            // [i.e. the number of bytes in the unit corresponding to mag]
            var adjustedSize = (decimal)value / (1L << (mag * 10));

            // make adjustment when the value is large enough that
            // it would round up to 1000 or more
            if (Math.Round(adjustedSize, decimalPlaces) >= 1000)
            {
                mag += 1;
                adjustedSize /= 1024;
            }

            return string.Format("{0:n" + decimalPlaces + "} {1}", adjustedSize, _sizeSuffixes[mag]);
        }
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            return value is long size ? SizeSuffix(size, 0) : null;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    // Taken from MS Docs
    class PlainView : ViewBase
    {
        public static readonly DependencyProperty ItemContainerStyleProperty =
          ItemsControl.ItemContainerStyleProperty.AddOwner(typeof(PlainView));

        public Style ItemContainerStyle
        {
            get => (Style)GetValue(ItemContainerStyleProperty);
            set => SetValue(ItemContainerStyleProperty, value);
        }

        public static readonly DependencyProperty ItemTemplateProperty =
            ItemsControl.ItemTemplateProperty.AddOwner(typeof(PlainView));

        public DataTemplate ItemTemplate
        {
            get => (DataTemplate)GetValue(ItemTemplateProperty);
            set => SetValue(ItemTemplateProperty, value);
        }

        public static readonly DependencyProperty ItemWidthProperty =
            WrapPanel.ItemWidthProperty.AddOwner(typeof(PlainView));

        public double ItemWidth
        {
            get => (double)GetValue(ItemWidthProperty);
            set => SetValue(ItemWidthProperty, value);
        }

        public static readonly DependencyProperty ItemHeightProperty =
            WrapPanel.ItemHeightProperty.AddOwner(typeof(PlainView));

        public double ItemHeight
        {
            get => (double)GetValue(ItemHeightProperty);
            set => SetValue(ItemHeightProperty, value);
        }

        protected override object DefaultStyleKey => new ComponentResourceKey(GetType(), "PlainViewResourceId");
    }

    /// <summary>
    /// Interaction logic for AssetBrowserView.xaml
    /// </summary>
    public partial class AssetBrowserView : UserControl, IDisposable
    {

        public SelectionMode SelectionMode
        {
            get => (SelectionMode)GetValue(SelectionModeProperty);
            set => SetValue(SelectionModeProperty, value);
        }

        public static readonly DependencyProperty SelectionModeProperty =
            DependencyProperty.Register(nameof(SelectionMode), typeof(SelectionMode), typeof(AssetBrowserView), new PropertyMetadata(SelectionMode.Extended));

        public FileAccess FileAccess
        {
            get => (FileAccess)GetValue(FileAccessProperty);
            set => SetValue(FileAccessProperty, value);
        }

        public static readonly DependencyProperty FileAccessProperty =
            DependencyProperty.Register(nameof(FileAccess), typeof(FileAccess), typeof(AssetBrowserView), new PropertyMetadata(FileAccess.ReadWrite));

        internal AssetFileInfo SelectedItem
        {
            get => (AssetFileInfo)GetValue(SelectedItemProperty);
            set => SetValue(SelectedItemProperty, value);
        }

        public static readonly DependencyProperty SelectedItemProperty =
            DependencyProperty.Register(nameof(SelectedItem), typeof(AssetFileInfo), typeof(AssetBrowserView), new PropertyMetadata(null));


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
                }
                else if (header.Column.ActualWidth > 250)
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
            if (sender is not GridViewColumnHeader column) return;
            if(column.Tag == null) return;
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

        private void OnFolderViewSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = folderListView.SelectedItem as AssetFileInfo;
            SelectedItem = item?.IsDirectory == true ? null : item;
        }

        public void Dispose()
        {
            if (Application.Current?.MainWindow != null)
            {
                Application.Current.MainWindow.DataContextChanged -= OnProjectChanged;
            }

            (DataContext as AssetBrowser)?.Dispose();
            DataContext = null;
        }
    }
}
