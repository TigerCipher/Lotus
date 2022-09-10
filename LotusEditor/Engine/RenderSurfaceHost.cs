using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;
using System.Windows.Interop;
using LotusEditor.DllWrapper;
using LotusEditor.Utility;
using Microsoft.VisualBasic.Devices;
using Mouse = System.Windows.Input.Mouse;

namespace LotusEditor.Engine
{
    internal class RenderSurfaceHost : HwndHost
    {

        private readonly int _width = 800;
        private readonly int _height = 600;
        private IntPtr _renderWindowHandle = IntPtr.Zero;
        private DelayEventTimer _resizeTimer;

        public int SurfaceId { get; private set; } = ID.INVALID_ID;

        public RenderSurfaceHost(double width, double height)
        {
            _width = (int)width;
            _height = (int)height;
            _resizeTimer = new DelayEventTimer(TimeSpan.FromMilliseconds(250.0));
            _resizeTimer.Triggered += Resize;
        }

        private void Resize(object sender, DelayEventTimerArgs e)
        {
            e.RepeatEvent = Mouse.LeftButton == MouseButtonState.Pressed;
            if (!e.RepeatEvent)
            {
                Logger.Info("Resized");
                EngineAPI.ResizeRenderSurface(SurfaceId);
            }
        }

        // [DllImport("user32.dll", EntryPoint = "CreateWindowEx", CharSet = CharSet.Unicode)]
        // internal static extern IntPtr CreateWindowEx(int dwExStyle,
        //                                       string lpszClassName,
        //                                       string lpszWindowName,
        //                                       int style,
        //                                       int x, int y,
        //                                       int width, int height,
        //                                       IntPtr hwndParent,
        //                                       IntPtr hMenu,
        //                                       IntPtr hInst,
        //                                       [MarshalAs(UnmanagedType.AsAny)] object pvParam);

        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            // IntPtr hwndHost = CreateWindowEx(0, "static", "",
            //                0x40000000 | 0x10000000 | 0x02000000,
            //                0, 0,
            //                _width, _height,
            //                hwndParent.Handle,
            //                IntPtr.Zero,
            //                IntPtr.Zero,
            //                0);
            SurfaceId = EngineAPI.CreateRenderSurface(hwndParent.Handle, _width, _height);
            // SurfaceId = EngineAPI.CreateRenderSurface(hwndHost, _width, _height);
            Debug.Assert(ID.IsValid(SurfaceId));
            _renderWindowHandle = EngineAPI.GetWindowHandle(SurfaceId);
            Debug.Assert(_renderWindowHandle != IntPtr.Zero);

            // return new HandleRef(this, hwndHost);
            return new HandleRef(this, _renderWindowHandle);
        }

        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            EngineAPI.RemoveRenderSurface(SurfaceId);
            SurfaceId = ID.INVALID_ID;
            _renderWindowHandle = IntPtr.Zero;
        }

        public void Resize()
        {
            _resizeTimer.Trigger();
        }
    }
}
