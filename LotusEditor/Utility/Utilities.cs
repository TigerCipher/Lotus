using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;
using System.Windows.Threading;

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
        public object Data { get; set; }

        public DelayEventTimerArgs(object data)
        {
            Data = data;
        }
    }

    internal class DelayEventTimer
    {
        private readonly DispatcherTimer _timer;
        private readonly TimeSpan _delay;
        private DateTime _lastTime = DateTime.Now;
        private object _data;

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
            _timer.IsEnabled = evtArgs.RepeatEvent;
        }

        public void Trigger(object data = null)
        {
            _data = data;
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
    }
}
