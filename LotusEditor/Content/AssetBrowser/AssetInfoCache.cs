using LotusEditor.Utility;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LotusEditor.Content
{
    static class AssetInfoCache
    {
        private static readonly object _lock = new();
        private static string _cacheFilePath = string.Empty;
        private static readonly Dictionary<string, AssetFileInfo> _assetInfoCache = new();
        private static bool _isDirty;

        public static void Reset(string projectPath)
        {
            lock (_lock)
            {
                if (!string.IsNullOrEmpty(_cacheFilePath) && _isDirty)
                {
                    SaveInfoCache();
                    _cacheFilePath = string.Empty;
                    _assetInfoCache.Clear();
                    _isDirty = false;
                }

                if (!string.IsNullOrEmpty(projectPath))
                {
                    Debug.Assert(Directory.Exists(projectPath));
                    _cacheFilePath = $@"{projectPath}.Lotus\AssetInfoCache.bin";
                    LoadInfoCache();
                }
            }
        }

        public static void Save() => Reset(string.Empty);

        public static AssetFileInfo Add(string file)
        {
            lock (_lock)
            {
                var fileInfo = new FileInfo(file);
                Debug.Assert(!fileInfo.IsDirectory());

                if (!_assetInfoCache.ContainsKey(file) ||
                    _assetInfoCache[file].DateModified.IsOlder(fileInfo.LastWriteTime))
                {
                    var info = AssetRegistry.GetAssetInfo(file) ?? Asset.GetAssetInfo(file);
                    Debug.Assert(info != null);
                    _assetInfoCache[file] = new AssetFileInfo(file, info.Icon);
                    _isDirty = true;
                }

                Debug.Assert(_assetInfoCache.ContainsKey(file));
                return _assetInfoCache[file];
            }
        }

        private static void SaveInfoCache()
        {
            try
            {
                using var writer = new BinaryWriter(File.Open(_cacheFilePath, FileMode.Create, FileAccess.Write));
                writer.Write(_assetInfoCache.Keys.Count);
                foreach (var key in _assetInfoCache.Keys)
                {
                    var info = _assetInfoCache[key];
                    writer.Write(key);
                    writer.Write(info.DateModified.ToBinary());
                    writer.Write(info.Icon.Length);
                    writer.Write(info.Icon);
                }

                _isDirty = false;
            }
            catch (Exception ex)
            {
                Logger.Warn($"Failed to read Asset Browser cache file. An exception occurred: {ex.Message}");
            }
        }

        private static void LoadInfoCache()
        {
            if (!File.Exists(_cacheFilePath)) return;

            try
            {
                using var reader = new BinaryReader(File.Open(_cacheFilePath, FileMode.Open, FileAccess.Read));
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
            catch (Exception ex)
            {
                Logger.Warn($"Failed to read Asset Browser cache file. An exception occurred: {ex.Message}");
                _assetInfoCache.Clear();
            }
        }
    }
}
