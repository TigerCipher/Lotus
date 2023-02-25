using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Threading;
using LotusEditor.Content;
using Microsoft.VisualBasic;

namespace LotusEditor.Utility
{
    internal static class MathHelper
    {
        public static float Epsilon => 0.00001f;

        public static bool IsEqual(this float value, float other)
        {
            return Math.Abs(value - other) < Epsilon;
        }

        public static bool IsEqual(this float? value, float? other)
        {
            if (!value.HasValue || !other.HasValue) return false;
            return Math.Abs(value.Value - other.Value) < Epsilon;
        }

        public static long AlignSizeUp(long size, long alignment)
        {
            Debug.Assert(alignment > 0, "Alignment must be non-zero");
            long mask = alignment - 1;
            Debug.Assert((alignment & mask) == 0, "Alignment must be a power of 2");

            return (size + mask) & ~mask;
        }

        public static long AlignSizeDown(long size, long alignment)
        {
            Debug.Assert(alignment > 0, "Alignment must be non-zero");
            long mask = alignment - 1;
            Debug.Assert((alignment & mask) == 0, "Alignment must be a power of 2");

            return (size & ~mask);
        }
    }

    internal static class ID
    {
        public static int INVALID_ID => -1; // int because entity ids are currently u32's. This will not work for u64
        public static bool IsValid(int id) => id != INVALID_ID;
    }

    internal class StringUtil
    {
        private static readonly Regex _whitespace = new Regex(@"\s+");

        public static string ReplaceWhitespace(string input, string replacement)
        {
            return _whitespace.Replace(input, replacement);
        }
    }


    internal class DelayEventTimerArgs : EventArgs
    {
        public bool RepeatEvent { get; set; }
        public IEnumerable<object> Data { get; set; }

        public DelayEventTimerArgs(IEnumerable<object> data)
        {
            Data = data;
        }
    }

    internal class DelayEventTimer
    {
        private readonly DispatcherTimer _timer;
        private readonly TimeSpan _delay;
        private readonly List<object> _data = new();
        private DateTime _lastTime = DateTime.Now;

        public event EventHandler<DelayEventTimerArgs> Triggered;

        public DelayEventTimer(TimeSpan delay, DispatcherPriority priority = DispatcherPriority.Normal)
        {
            _delay = delay;
            _timer = new DispatcherTimer(priority)
            {
                Interval = TimeSpan.FromMilliseconds(delay.TotalMilliseconds * 0.5)
            };
            _timer.Tick += OnTimerTick;
        }

        private void OnTimerTick(object sender, EventArgs e)
        {
            if ((DateTime.Now - _lastTime) < _delay) return;
            var evtArgs = new DelayEventTimerArgs(_data);
            Triggered?.Invoke(this, evtArgs);

            if (!evtArgs.RepeatEvent)
            {
                _data.Clear();
            }

            _timer.IsEnabled = evtArgs.RepeatEvent;
        }

        public void Trigger(object data = null)
        {
            if (data != null)
            {
                _data.Add(data);
            }
            _lastTime = DateTime.Now;
            _timer.IsEnabled = true;
        }

        public void Disable()
        {
            _timer.IsEnabled = false;
        }


    }

    static class VisualExtensions
    {
        public static T FindVisualParent<T>(this DependencyObject depObj) where T : DependencyObject
        {
            if (depObj is not Visual) return null;

            var parent = VisualTreeHelper.GetParent(depObj);
            while (parent != null)
            {
                if (parent is T type)
                {
                    return type;
                }
                parent = VisualTreeHelper.GetParent(parent);
            }
            return null;
        }
    }


    public static class ContentUtil
    {
        public static string GetRandomString(int length = 8)
        {
            if (length <= 0) length = 8;
            var n = length / 11;
            var sb = new StringBuilder();
            for (int i = 0; i <= n; ++i)
            {
                sb.Append(Path.GetRandomFileName().Replace(".", ""));
            }

            return sb.ToString(0, length);
        }

        public static string FixFilename(string name)
        {
            var path = new StringBuilder(name[..(name.LastIndexOf(Path.DirectorySeparatorChar) + 1)]);
            var file = new StringBuilder(name[(name.LastIndexOf(Path.DirectorySeparatorChar) + 1)..]);
            foreach (var c in Path.GetInvalidPathChars())
            {
                path.Replace(c, '_');
            }
            foreach (var c in Path.GetInvalidFileNameChars())
            {
                file.Replace(c, '_');
            }

            return path.Append(file).ToString();
        }

        public static bool IsOlder(this DateTime date, DateTime other) => date < other;

        public static byte[] ComputeHash(byte[] data, int offset = 0, int count = 0)
        {
            if (!(data?.Length > 0)) return null;
            using var sha = SHA256.Create();
            return sha.ComputeHash(data, offset, count > 0 ? count : data.Length);

        }

        public static bool IsDirectory(string path)
        {
            try
            {
                return File.GetAttributes(path).HasFlag(FileAttributes.Directory);
            }
            catch (Exception ex)
            {
                Debug.WriteLine($"Error occurred while checking file attributes: {ex.Message}");
                return false;
            }
        }

        public static bool IsDirectory(this FileInfo info) => info.Attributes.HasFlag(FileAttributes.Directory);

        public static async Task ImportFilesAsync(string[] files, string dest)
        {
            try
            {
                Debug.Assert(!string.IsNullOrEmpty(dest));
                ContentWatcher.EnableFileWatcher(false);
                var tasks = files.Select(async file => await Task.Run(() => { Import(file, dest); }));
                await Task.WhenAll(tasks);
            }
            catch (Exception ex)
            {
                Logger.Error($"Failed to import files to {dest}");
                Logger.Error(ex.Message);
            }
            finally
            {
                ContentWatcher.EnableFileWatcher(true);
            }
        }

        private static void Import(string file, string dest)
        {
            Debug.Assert(!string.IsNullOrEmpty(file));
            if (IsDirectory(file)) return;
            if (!dest.EndsWith(Path.DirectorySeparatorChar))
                dest += Path.DirectorySeparatorChar;
            var name = Path.GetFileNameWithoutExtension(file).ToLower();
            var ext = Path.GetExtension(file).ToLower();

            Asset asset = null;

            switch (ext)
            {
                case ".fbx": asset = new Content.Geometry(); break;
                case ".bmp": break;
                case ".png": break;
                case ".jpg": break;
                case ".jpeg": break;
                case ".tiff": break;
                case ".tif": break;
                case ".tga": break;
                case ".wav": break;
                case ".ogg": break;
                default: break;
            }

            if (asset != null)
            {
                Import(asset, name, file, dest);
            }
        }

        private static void Import(Asset asset, string name, string file, string dest)
        {
            Debug.Assert(asset != null);
            asset.FullPath = dest + name + Asset.AssetFileExtension;

            if (!string.IsNullOrEmpty(file))
            {
                asset.Import(file);
            }

            asset.Save(asset.FullPath);
        }
    }
}
