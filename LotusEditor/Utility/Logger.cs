using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Net.Mime;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Data;

namespace LotusEditor.Utility
{

    [Flags]
    enum Level
    {
        Info = 0x01,
        Warning = 0x02,
        Error = 0x04
    }

    class LogMessage
    {
        public DateTime Time { get; }
        public Level MsgLevel { get; }
        public string Message { get; }
        public string File { get; }
        public string Caller { get; }
        public int Line { get; }
        public string MetaData => $"{File}:{Caller} ({Line})";

        public LogMessage(Level level, string msg, string file, string caller, int line)
        {
            Time = DateTime.Now;
            MsgLevel = level;
            Message = msg;
            File = Path.GetFileName(file);
            Caller = caller;
            Line = line;
        }
    }

    internal static class Logger
    {
        private static readonly ObservableCollection<LogMessage> _messages = new();

        public static ReadOnlyObservableCollection<LogMessage> Messages { get; } = new(_messages);

        public static CollectionViewSource FilteredMessages { get; } = new() { Source = Messages };

        private static int _messageFilter = (int)(Level.Info | Level.Warning | Level.Error);


        static Logger()
        {
            FilteredMessages.Filter += (sender, args) =>
            {
                var level = (int)(args.Item as LogMessage).MsgLevel;
                args.Accepted = (level & _messageFilter) != 0;
            };

            Info("Test info msg");
            Warn("Warn info msg");
            Error("Error info msg");
            Error("Error info msg");
            Warn("Warn info msg");
            Info("Test info msg");
            Info("Test info msg");
            Warn("Test info msg");
            Info("Test info msg");
        }

        public static async void Log(Level level, string msg, [CallerFilePath]string file="", [CallerMemberName]string caller="", [CallerLineNumber]int line=0)
        {
            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _messages.Add(new LogMessage(level, msg, file, caller, line));
            }));
        }

        public static async void Clear()
        {
            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _messages.Clear();
            }));
        }

        public static async void Info(string msg, [CallerFilePath] string file = "",
            [CallerMemberName] string caller = "", [CallerLineNumber] int line = 0)
        {
            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _messages.Add(new LogMessage(Level.Info, msg, file, caller, line));
            }));
        }

        public static async void Warn(string msg, [CallerFilePath] string file = "",
            [CallerMemberName] string caller = "", [CallerLineNumber] int line = 0)
        {
            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _messages.Add(new LogMessage(Level.Warning, msg, file, caller, line));
            }));
        }

        public static async void Error(string msg, [CallerFilePath] string file = "",
            [CallerMemberName] string caller = "", [CallerLineNumber] int line = 0)
        {
            await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
            {
                _messages.Add(new LogMessage(Level.Error, msg, file, caller, line));
            }));
        }

        public static void SetMessageFilter(int mask)
        {
            _messageFilter = mask;
            FilteredMessages.View.Refresh();
        }


    }
}
