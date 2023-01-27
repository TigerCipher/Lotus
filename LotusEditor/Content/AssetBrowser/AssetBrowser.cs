using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Printing.IndexedProperties;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using LotusEditor.GameProject;
using LotusEditor.Utility;

namespace LotusEditor.Content
{

    sealed class AssetFileInfo
    {
        public static int IconWidth => 90;
        public byte[] Icon { get; }
        public byte[] IconSmall { get; }
        public string FullPath { get; }
        public string FileName => Path.GetFileNameWithoutExtension(FullPath);
        public bool IsDirectory { get; }
        public DateTime DateModified { get; }
        public long? Size { get; }

        public AssetFileInfo(string fullPath, byte[] icon = null, byte[] iconSmall = null, DateTime? lastModified = null)
        {
            Debug.Assert(File.Exists(fullPath) || Directory.Exists(fullPath));
            var info = new FileInfo(fullPath);
            IsDirectory = ContentUtil.IsDirectory(fullPath);
            DateModified = lastModified ?? info.LastWriteTime;
            Size = IsDirectory ? null : info.Length;
            Icon = icon;
            IconSmall = iconSmall ?? icon;
            FullPath = fullPath;
        }
    }

    class AssetBrowser : ViewModelBase, IDisposable
    {
        public string AssetsFolder { get; }

        private readonly ObservableCollection<AssetFileInfo> _folderAssets = new();
        public ReadOnlyObservableCollection<AssetFileInfo> FolderAssets { get; }

        private static readonly DelayEventTimer _refreshTimer = new(TimeSpan.FromMilliseconds(250));

        private string _selectedFolder;

        public string SelectedFolder
        {
            get => _selectedFolder;
            set
            {
                if (_selectedFolder == value) return;
                _selectedFolder = value;
                if (!string.IsNullOrEmpty(_selectedFolder))
                {
                    _ = GetFolderAssets();
                }
                OnPropertyChanged(nameof(SelectedFolder));
            }
        }

        

        public AssetBrowser(Project project)
        {
            Debug.Assert(project != null);
            var assetFolder = project.ContentPath;
            assetFolder = Path.TrimEndingDirectorySeparator(assetFolder);
            AssetsFolder = assetFolder;
            SelectedFolder = assetFolder;
            FolderAssets = new ReadOnlyObservableCollection<AssetFileInfo>(_folderAssets);

            ContentWatcher.ContentModified += OnAssetModified;
            _refreshTimer.Triggered += Refresh;


        }


        private void Refresh(object sender, DelayEventTimerArgs e)
        {
            _ = GetFolderAssets();
        }

        private async Task GetFolderAssets()
        {
            var folderAssets = new List<AssetFileInfo>();
            await Task.Run(() =>
            {
                folderAssets = GetFolderAssets(SelectedFolder);
            });

            _folderAssets.Clear();
            folderAssets.ForEach(x => _folderAssets.Add(x));
        }

        private static List<AssetFileInfo> GetFolderAssets(string path)
        {
            Debug.Assert(!string.IsNullOrEmpty(path));

            var folderAssets = new List<AssetFileInfo>();

            try
            {
                // Sub folders
                folderAssets.AddRange(Directory.GetDirectories(path).Select(dir => new AssetFileInfo(dir)));

                // Files
                    foreach (var file in Directory.GetFiles(path, $"*{Asset.AssetFileExtension}"))
                    {
                        folderAssets.Add(AssetInfoCache.Add(file));
                    }
            }
            catch (Exception e)
            {
                Logger.Error(e.Message);
            }

            return folderAssets;
        }

        public void Dispose()
        {
            ContentWatcher.ContentModified -= OnAssetModified;
            AssetInfoCache.Save();
        }

        private void OnAssetModified(object sender, ContentModifiedEventArgs e)
        {
            if (Path.GetDirectoryName(e.FullPath) != SelectedFolder) return;
            _refreshTimer.Trigger();

        }

    }
}
