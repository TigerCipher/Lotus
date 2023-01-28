using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Windows;
using LotusEditor.Utility;

namespace LotusEditor.Content
{
    static class AssetRegistry
    {
        private static readonly Dictionary<string, AssetInfo> _assetDictionary = new();
        private static readonly ObservableCollection<AssetInfo> _assets = new();

        public static ReadOnlyObservableCollection<AssetInfo> Assets { get; } = new(_assets);



        private static void UnregisterAsset(string file)
        {
            if (!_assetDictionary.ContainsKey(file)) return;

            _assets.Remove(_assetDictionary[file]);
            _assetDictionary.Remove(file);
            Logger.Info($"Unregistered asset: [{file}]");
        }

        private static async void OnAssetModified(object sender, ContentModifiedEventArgs e)
        {
            if (ContentUtil.IsDirectory(e.FullPath))
            {
                RegisterAllAssets(e.FullPath);
            }
            else if (File.Exists(e.FullPath))
            {
                RegisterAsset(e.FullPath);
            }

            _assets.Where(x => !File.Exists(x.FullPath)).ToList().ForEach(x => UnregisterAsset(x.FullPath));
        }

        public static void Reset(string assetsFolder)
        {
            ContentWatcher.ContentModified -= OnAssetModified;

            _assetDictionary.Clear();
            _assets.Clear();
            Debug.Assert(Directory.Exists(assetsFolder));
            RegisterAllAssets(assetsFolder);

            ContentWatcher.ContentModified += OnAssetModified;
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
        }


        private static void RegisterAsset(string file)
        {
            // if (!File.Exists(file)) return;
            Debug.Assert(File.Exists(file), $"Asset File: {file}");
            try
            {
                var fileInfo = new FileInfo(file);
                if (!_assetDictionary.ContainsKey(file) ||
                    _assetDictionary[file].RegisterTime.IsOlder(fileInfo.LastWriteTime))
                {
                    var info = Asset.GetAssetInfo(file);
                    Debug.Assert(info != null);

                    info.RegisterTime = DateTime.Now;

                    _assetDictionary[file] = info;
                    Debug.Assert(_assetDictionary.ContainsKey(file));
                    _assets.Add(_assetDictionary[file]);
                    Logger.Info($"Registered asset: [{file}]");
                }


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
