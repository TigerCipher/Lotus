using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using LotusEditor.Utility;

namespace LotusEditor.Engine
{
    /// <summary>
    /// Interaction logic for RenderSurfaceView.xaml
    /// </summary>
    public partial class RenderSurfaceView : UserControl, IDisposable
    {

        private enum WinMsg
        {
            WM_SIZE = 0x0005,
            WM_SIZING = 0x0214,
            WM_ENTERSIZEMOVE = 0x0231,
            WM_EXITSIZEMOVE = 0x0232,
        }

        private RenderSurfaceHost _host;
        private bool _canResize = true;
        private bool _moved = false;

        private bool _disposedValue;

        public RenderSurfaceView()
        {
            InitializeComponent();
            Loaded += OnRenderSurfaceLoaded;
        }

        private void OnRenderSurfaceLoaded(object sender, RoutedEventArgs e)
        {
            Loaded -= OnRenderSurfaceLoaded;

            _host = new RenderSurfaceHost(ActualWidth, ActualHeight);
            _host.MessageHook += new HwndSourceHook(HostMsgFilter);
            Content = _host;

            var window = this.FindVisualParent<Window>();
            Debug.Assert(window != null);

            var helper = new WindowInteropHelper(window);
            if (helper.Handle != null)
            {
                HwndSource.FromHwnd(helper.Handle)?.AddHook(HwndMsgHook);
            }
        }

        private IntPtr HwndMsgHook(IntPtr hwnd, int msg, IntPtr wparam, IntPtr lparam, ref bool handled)
        {
            switch ((WinMsg)msg)
            {
                case WinMsg.WM_SIZING:
                    _canResize = false;
                    _moved = false;
                    break;
                case WinMsg.WM_ENTERSIZEMOVE:
                    _moved = true;
                    break;
                case WinMsg.WM_EXITSIZEMOVE:
                    _canResize = true;
                    if(!_moved) _host.Resize();
                    break;
                default: break;
            }

            return IntPtr.Zero;
        }

        private IntPtr HostMsgFilter(IntPtr hwnd, int msg, IntPtr wparam, IntPtr lparam, ref bool handled)
        {
            switch ((WinMsg)msg)
            {
                case WinMsg.WM_SIZE:
                    if(_canResize) _host.Resize();
                    break;
                case WinMsg.WM_SIZING: throw new Exception();
                case WinMsg.WM_ENTERSIZEMOVE: throw new Exception();
                case WinMsg.WM_EXITSIZEMOVE: throw new Exception();
            }

            return IntPtr.Zero;
        }


        protected virtual void Dispose(bool disposing)
        {
            if (_disposedValue) return;
            if (disposing)
            {
                _host.Dispose();
            }

            _disposedValue = true;
        }

        public void Dispose()
        {
            Dispose(disposing: true);
            GC.SuppressFinalize(this);
        }
    }
}
