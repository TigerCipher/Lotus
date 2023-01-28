using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using LotusEditor.Utility;

namespace LotusEditor.Content
{
    public class ContentModifiedEventArgs : EventArgs
    {
        public string FullPath { get; }

        public ContentModifiedEventArgs(string path)
        {
            FullPath = path;
        }
    }

    static class ContentWatcher
    {
        private static readonly FileSystemWatcher _contentWatcher = new()
        {
            IncludeSubdirectories = true,
            Filter = "",
            NotifyFilter = NotifyFilters.CreationTime |
                           NotifyFilters.DirectoryName |
                           NotifyFilters.FileName |
                           NotifyFilters.LastWrite
        };


        private static readonly DelayEventTimer _refreshTimer = new(TimeSpan.FromMilliseconds(250));

        private static int _fileWatcherEnableCounter = 0;
        public static event EventHandler<ContentModifiedEventArgs> ContentModified;

        static ContentWatcher()
        {


            _contentWatcher.Changed += OnContentModified;
            _contentWatcher.Created += OnContentModified;
            _contentWatcher.Deleted += OnContentModified;
            _contentWatcher.Renamed += OnContentModified;
            // _contentWatcher.EnableRaisingEvents = true;

            _refreshTimer.Triggered += Refresh;
        }

        private static void Refresh(object sender, DelayEventTimerArgs e)
        {
            if (_fileWatcherEnableCounter > 0)
            {
                e.RepeatEvent = true;
                return;
            }

            e.Data.Cast<FileSystemEventArgs>().GroupBy(x => x.FullPath).Select(x => x.First()).ToList()
                    .ForEach(x => ContentModified?.Invoke(null, new ContentModifiedEventArgs(x.FullPath)));
        }

        private static async void OnContentModified(object sender, FileSystemEventArgs e)
        {
            await Application.Current.Dispatcher.BeginInvoke(new Action(() => _refreshTimer.Trigger(e)));
        }

        public static void Reset(string contentFolder, string projectPath)
        {
            _contentWatcher.EnableRaisingEvents = false;

            AssetInfoCache.Reset(projectPath);

            if (!string.IsNullOrEmpty(contentFolder))
            {
                Debug.Assert(Directory.Exists(contentFolder));
                _contentWatcher.Path = contentFolder;
                _contentWatcher.EnableRaisingEvents = true;
                AssetRegistry.Reset(contentFolder);
            }
        }

        public static void EnableFileWatcher(bool enabled)
        {
            if (_fileWatcherEnableCounter > 0 && enabled)
            {
                --_fileWatcherEnableCounter;
            }
            else if(!enabled)
            {
                ++_fileWatcherEnableCounter;
            }
        }
    }
}
