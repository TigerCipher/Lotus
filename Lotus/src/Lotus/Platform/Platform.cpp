// ------------------------------------------------------------------------------
//
// Lotus
//    Copyright 2022 Matthew Rogers
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//
// File Name: Platform.cpp
// Date File Created: 8/28/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "Platform.h"

namespace lotus::platform
{

namespace
{

    struct window_info
    {
        HWND  hwnd = nullptr;
        RECT  clientArea { 0, 0, 1920, 1080 };
        RECT  fullscreenArea {};
        POINT topLeft { 0, 0 };
        DWORD style      = WS_VISIBLE;
        bool  fullscreen = false;
        bool  closed     = false;
    };

    utl::vector<window_info> windows;

    utl::vector<u32> availableSlots;

    uint32 add_to_windows(window_info info)
    {
        u32 id = InvalidIdU32;
        if (availableSlots.empty())
        {
            id = (u32) windows.size();
            windows.emplace_back(info);
        } else
        {
            id = availableSlots.back();
            availableSlots.pop_back();
            LASSERT(id != InvalidIdU32);
            windows [ id ] = info;
        }

        return id;
    }

    void remove_from_windows(uint32 id)
    {
        LASSERT(id < windows.size());
        availableSlots.emplace_back(id);
    }

    window_info& get_from_id(window_id id)
    {
        LASSERT(id < windows.size());
        LASSERT(windows [ id ].hwnd);
        return windows [ id ];
    }

    window_info& get_from_handle(const window_handle handle)
    {
        const window_id id { (id::id_type) GetWindowLongPtr(handle, GWLP_USERDATA) };
        return get_from_id(id);
    }

    LRESULT CALLBACK internal_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        window_info* info = nullptr;
        switch (msg)
        {
        case WM_DESTROY: get_from_handle(hwnd).closed = true; break;
        case WM_EXITSIZEMOVE: info = &get_from_handle(hwnd); break;
        case WM_SIZE:
            if (wParam == SIZE_MAXIMIZED) info = &get_from_handle(hwnd);
            break;
        case WM_SYSCOMMAND:
            if (wParam == SC_RESTORE) info = &get_from_handle(hwnd);
            break;
        default: break;
        }

        if (info)
        {
            LASSERT(info->hwnd);
            GetClientRect(info->hwnd, info->fullscreen ? &info->fullscreenArea : &info->clientArea);
        }

        LONG_PTR longptr = GetWindowLongPtr(hwnd, 0);
        return longptr ? ((window_proc) longptr)(hwnd, msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
    }


    void resize_window(const window_info& info, const RECT& area)
    {
        RECT winrect = area;
        AdjustWindowRect(&winrect, info.style, FALSE);
        const s32 width  = winrect.right - winrect.left;
        const s32 height = winrect.bottom - winrect.top;
        MoveWindow(info.hwnd, info.topLeft.x, info.topLeft.y, width, height, TRUE);
    }

    void resize_window(const window_id id, const uint32 width, const uint32 height)
    {
        window_info& info = get_from_id(id);
        RECT&        area = info.fullscreen ? info.fullscreenArea : info.clientArea;
        area.bottom       = area.top + height;
        area.right        = area.left + width;
        resize_window(info, area);
    }

    void set_window_fullscreen(window_id id, bool fullscreen)
    {
        window_info& info = get_from_id(id);
        if (info.fullscreen != fullscreen)
        {
            info.fullscreen = fullscreen;
            if (fullscreen)
            {
                GetClientRect(info.hwnd, &info.clientArea);
                RECT rect;
                GetWindowRect(info.hwnd, &rect);
                info.topLeft.x = rect.left;
                info.topLeft.y = rect.top;
                SetWindowLongPtr(info.hwnd, GWL_STYLE, 0);
                ShowWindow(info.hwnd, SW_MAXIMIZE);
            } else
            {
                SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
                resize_window(info, info.clientArea);
                ShowWindow(info.hwnd, SW_SHOWNORMAL);
            }
        }
    }

    bool is_window_fullscreen(const window_id id) { return get_from_id(id).fullscreen; }

    window_handle get_window_handle(const window_id id) { return get_from_id(id).hwnd; }

    void set_window_caption(const window_id id, const wchar_t* caption)
    {
        const window_info& info = get_from_id(id);
        SetWindowText(info.hwnd, caption);
    }

    vec2u get_window_size(const window_id id)
    {
        const window_info& info = get_from_id(id);
        const RECT&        area = info.fullscreen ? info.fullscreenArea : info.clientArea;

        return { (u32) area.right - (u32) area.left, (u32) area.bottom - (u32) area.top };
    }

    vec4u get_window_rect(const window_id id)
    {
        const window_info& info = get_from_id(id);
        const RECT&        area = info.fullscreen ? info.fullscreenArea : info.clientArea;

        return { (u32) area.left, (u32) area.top, (u32) area.right, (u32) area.bottom };
    }

    bool is_window_closed(const window_id id) { return get_from_id(id).closed; }

} // namespace

Window create_window(const window_create_info* const info)
{
    window_proc   callback = info ? info->callback : nullptr;
    window_handle parent   = info ? info->parent : nullptr;

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.cbSize        = sizeof(WNDCLASSEX); // Size of the struct
    wc.style         = CS_HREDRAW | CS_VREDRAW; // redraws whenever window size changes
    wc.lpfnWndProc   = internal_window_proc;
    wc.cbClsExtra    = NULL;
    wc.cbWndExtra    = callback ? sizeof(callback) : 0;
    wc.hInstance     = nullptr;
    wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(76, 26, 48));
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = L"LotusWindow";
    wc.hIconSm       = LoadIcon(nullptr, IDI_APPLICATION);

    RegisterClassEx(&wc);

    window_info winInfo;
    winInfo.clientArea.right = info && info->width ? winInfo.clientArea.left + info->width : winInfo.clientArea.right;
    winInfo.clientArea.bottom =
        info && info->height ? winInfo.clientArea.top + info->height : winInfo.clientArea.bottom;

    RECT rect { winInfo.clientArea };

    winInfo.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;

    AdjustWindowRect(&rect, winInfo.style, FALSE);

    const wchar_t* caption = info && info->caption ? info->caption : L"Lotus Game";
    const s32      left    = info ? info->left : winInfo.topLeft.x;
    const s32      top     = info ? info->top : winInfo.topLeft.y;
    const s32      width   = rect.right - rect.left;
    const s32      height  = rect.bottom - rect.top;


    // extended style, window class name, instance title, window style, x pos, y pos, width, height, menu, hinstance, extra params
    winInfo.hwnd = CreateWindowEx(0, wc.lpszClassName, caption, winInfo.style, left, top, width, height, parent,
                                  nullptr, nullptr, nullptr);

    if (winInfo.hwnd)
    {
        L_DBG(SetLastError(0));
        const window_id id { add_to_windows(winInfo) };

        SetWindowLongPtr(winInfo.hwnd, GWLP_USERDATA, (LONG_PTR) id);

        if (callback) SetWindowLongPtr(winInfo.hwnd, 0, (LONG_PTR) callback);
        LASSERT(GetLastError() == 0);

        ShowWindow(winInfo.hwnd, SW_SHOWNORMAL);
        UpdateWindow(winInfo.hwnd);

        return Window(id);
    }

    return {};
}

void remove_window(const window_id id)
{
    const window_info& info = get_from_id(id);
    DestroyWindow(info.hwnd);
    remove_from_windows(id);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Window Class Implementations ////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Window::SetFullscreen(const bool fullscreen) const
{
    LASSERT(IsValid());
    set_window_fullscreen(mId, fullscreen);
}

bool Window::IsFullscreen() const
{
    LASSERT(IsValid());
    return is_window_fullscreen(mId);
}

void Window::SetCaption(const wchar_t* caption) const
{
    LASSERT(IsValid());
    set_window_caption(mId, caption);
}

vec2u Window::Size() const
{
    LASSERT(IsValid());
    return get_window_size(mId);
}

vec4u Window::Rect() const
{
    LASSERT(IsValid());
    return get_window_rect(mId);
}

void Window::Resize(const uint32 width, const uint32 height) const
{
    LASSERT(IsValid());
    resize_window(mId, width, height);
}

uint32 Window::Width() const { return Size().x; }

uint32 Window::Height() const { return Size().y; }

bool Window::IsClosed() const
{
    LASSERT(IsValid());
    return is_window_closed(mId);
}

void* Window::Handle() const
{
    LASSERT(IsValid());
    return get_window_handle(mId);
}

} // namespace lotus::platform
