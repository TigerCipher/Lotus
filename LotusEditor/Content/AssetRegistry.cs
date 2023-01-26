using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net.Mime;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using LotusEditor.Utility;

namespace LotusEditor.Content
{
    static class AssetRegistry
    {
        private static readonly Dictionary<string, AssetInfo> _assetDictionary = new();
        private static readonly ObservableCollection<AssetInfo> _assets = new();

        private static readonly FileSystemWatcher _assetsWatcher = new()
        {
            IncludeSubdirectories = true,
            Filter = "",
            NotifyFilter = NotifyFilters.CreationTime |
                           NotifyFilters.DirectoryName |
                           NotifyFilters.FileName |
                           NotifyFilters.LastWrite
        };

        public static ReadOnlyObservableCollection<AssetInfo> Assets { get; } = new(_assets);

        private static readonly DelayEventTimer _refreshTimer = new(TimeSpan.FromMilliseconds(250));


        static AssetRegistry()
        {
            _assetsWatcher.Changed += OnAssetModified;
            _assetsWatcher.Created += OnAssetModified;
            _assetsWatcher.Deleted += OnAssetModified;
            _assetsWatcher.Renamed += OnAssetModified;

            _refreshTimer.Triggered += Refresh;
        }

        private static void Refresh(object sender, DelayEventTimerArgs e)
        {
            foreach (var item in e.Data)
            {
                if (item is not FileSystemEventArgs eventArgs) continue;

                if (eventArgs.ChangeType == WatcherChangeTypes.Deleted)
                {
                    UnregisterAsset(eventArgs.FullPath);
                }
                else
                {
                    RegisterAsset(eventArgs.FullPath);
                    if (eventArgs.ChangeType == WatcherChangeTypes.Renamed)
                    {
                        _assetDictionary.Keys.Where(key => !File.Exists(key)).ToList().ForEach(file => UnregisterAsset(file));
                    }
                }
            }
        }

        private static void UnregisterAsset(string file)
        {
            if (!_assetDictionary.ContainsKey(file)) return;

            _assets.Remove(_assetDictionary[file]);
            _assetDictionary.Remove(file);
            Logger.Info($"Unregistered asset: [{file}]");
        }

        private static async void OnAssetModified(object sender, FileSystemEventArgs e)
        {
            if (Path.GetExtension(e.FullPath) != Asset.AssetFileExtension) return;

            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _refreshTimer.Trigger(e);
            }));
        }


        public static void Clear()
        {
            _assetsWatcher.EnableRaisingEvents = false;
            _assetDictionary.Clear();
            _assets.Clear();
        }

        public static void Reset(string assetsFolder)
        {
            Clear();
            Debug.Assert(Directory.Exists(assetsFolder));
            RegisterAllAssets(assetsFolder);
            _assetsWatcher.Path = assetsFolder;
            _assetsWatcher.EnableRaisingEvents = true;
        }

        private static void RegisterAllAssets(string assetsFolder)
        {
            Debug.Assert(Directory.Exists(assetsFolder));
            foreach (var entry in Directory.GetFileSystemEntries(assetsFolder))
            {
                if (ContentUtil.IsDirectory(entry))
                {
                    RegisterAllAssets(entry);
                }
                else
                {
                    RegisterAsset(entry);
                }
            }
            throw new NotImplementedException();
        }


        private static void RegisterAsset(string file)
        {
            try
            {
                var fileInfo = new FileInfo(file);
                if (_assetDictionary.ContainsKey(file) &&
                    !_assetDictionary[file].RegisterTime.IsOlder(fileInfo.LastWriteTime)) return;

                var info = Asset.GetAssetInfo(file);
                Debug.Assert(info != null);

                info.RegisterTime = DateTime.Now;

                _assetDictionary[file] = info;
                Debug.Assert(_assetDictionary.ContainsKey(file));
                _assets.Add(_assetDictionary[file]);
                Logger.Info($"Registered asset: [{file}]");
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Failed to register asset {file}: {ex.Message}");
            }
        }

        public static AssetInfo GetAssetInfo(string file) =>
            _assetDictionary.ContainsKey(file) ? _assetDictionary[file] : null;

        public static AssetInfo GetAssetInfo(Guid guid) => _assets.FirstOrDefault(x => x.Guid == guid);

    }
}
