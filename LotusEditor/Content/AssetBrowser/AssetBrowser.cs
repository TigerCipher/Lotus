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
        private static readonly object _lock = new();

        private static readonly FileSystemWatcher _assetsWatcher = new()
        {
            IncludeSubdirectories = true,
            Filter = "",
            NotifyFilter = NotifyFilters.CreationTime  |
                           NotifyFilters.DirectoryName |
                           NotifyFilters.FileName      |
                           NotifyFilters.LastWrite
        };


        private static readonly DelayEventTimer _refreshTimer = new(TimeSpan.FromMilliseconds(250));

        public string AssetsFolder { get; }

        private readonly ObservableCollection<AssetFileInfo> _folderAssets = new();
        public ReadOnlyObservableCollection<AssetFileInfo> FolderAssets { get; }

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
                    GetFolderAssets();
                }
                OnPropertyChanged(nameof(SelectedFolder));
            }
        }

        private static string _cacheFilePath = string.Empty;
        private static readonly Dictionary<string, AssetFileInfo> _assetInfoCache = new();

        public AssetBrowser(Project project)
        {
            Debug.Assert(project != null);
            var assetFolder = project.ContentPath;
            assetFolder = Path.TrimEndingDirectorySeparator(assetFolder);
            AssetsFolder = assetFolder;
            SelectedFolder = assetFolder;
            FolderAssets = new ReadOnlyObservableCollection<AssetFileInfo>(_folderAssets);

            if (string.IsNullOrEmpty(_cacheFilePath))
            {
                _cacheFilePath = $@"{project.Path}.Lotus\AssetInfoCache.bin";
                LoadInfoCache(_cacheFilePath);
            }

            _assetsWatcher.Path = assetFolder;

            _assetsWatcher.Changed += OnAssetModified;
            _assetsWatcher.Created += OnAssetModified;
            _assetsWatcher.Deleted += OnAssetModified;
            _assetsWatcher.Renamed += OnAssetModified;
            _assetsWatcher.EnableRaisingEvents = true;

            _refreshTimer.Triggered += Refresh;
        }


        private void Refresh(object sender, DelayEventTimerArgs e)
        {
            GetFolderAssets();
        }

        private async void OnAssetModified(object sender, FileSystemEventArgs e)
        {
            if (Path.GetDirectoryName(e.FullPath) != SelectedFolder) return;

            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _refreshTimer.Trigger();
            }));
        }

        private async void GetFolderAssets()
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
                lock(_lock)
                {
                    foreach (var file in Directory.GetFiles(path, $"*{Asset.AssetFileExtension}"))
                    {
                        var fileInfo = new FileInfo(file);

                        if (!_assetInfoCache.ContainsKey(file) ||
                            _assetInfoCache[file].DateModified.IsOlder(fileInfo.LastWriteTime))
                        {
                            var info = AssetRegistry.GetAssetInfo(file) ?? Asset.GetAssetInfo(file);
                            Debug.Assert(info != null);
                            _assetInfoCache[file] = new AssetFileInfo(file, info.Icon);
                        }

                        Debug.Assert(_assetInfoCache.ContainsKey(file));
                        folderAssets.Add(_assetInfoCache[file]);
                    }
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
            ((IDisposable)_assetsWatcher).Dispose();
            if (string.IsNullOrEmpty(_cacheFilePath)) return;
            SaveInfoCache(_cacheFilePath);
            _cacheFilePath = string.Empty;
        }

        private void SaveInfoCache(string file)
        {
            lock (_lock)
            {
                using var writer = new BinaryWriter(File.Open(file, FileMode.Create, FileAccess.Write));
                writer.Write(_assetInfoCache.Keys.Count);
                foreach (var key in _assetInfoCache.Keys)
                {
                    var info = _assetInfoCache[key];
                    writer.Write(key);
                    writer.Write(info.DateModified.ToBinary());
                    writer.Write(info.Icon.Length);
                    writer.Write(info.Icon);
                }
            }
        }

        private void LoadInfoCache(string file)
        {
            if (!File.Exists(file)) return;

            try
            {
                lock(_lock)
                {
                    using var reader = new BinaryReader(File.Open(file, FileMode.Open, FileAccess.Read));
                    var numEntries = reader.ReadInt32();
                    _assetInfoCache.Clear();

                    for (var i = 0; i < numEntries; ++i)
                    {
                        var assetFile = reader.ReadString();
                        var date = DateTime.FromBinary(reader.ReadInt64());
                        var iconSize = reader.ReadInt32();
                        var icon = reader.ReadBytes(iconSize);

                        if (File.Exists(assetFile))
                        {
                            _assetInfoCache[assetFile] = new AssetFileInfo(assetFile, icon, null, date);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Logger.Warn($"Failed to read Asset Browser cache file. An exception occurred: {ex.Message}");
                _assetInfoCache.Clear();
            }
        }

    }
}
