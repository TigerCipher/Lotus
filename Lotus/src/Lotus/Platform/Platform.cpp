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

bool resized = false;

struct window_info
{
    HWND  hwnd = nullptr;
    RECT  client_area{ 0, 0, 1920, 1080 };
    RECT  fullscreen_area{};
    POINT top_left{ 0, 0 };
    DWORD style      = WS_VISIBLE;
    bool  fullscreen = false;
    bool  closed     = false;
};

utl::free_list<window_info> windows;

window_info& get_from_id(window_id id)
{
    assert(windows[id].hwnd);
    return windows[id];
}

window_info& get_from_handle(const window_handle handle)
{
    const window_id id{ (id::id_type) GetWindowLongPtr(handle, GWLP_USERDATA) };
    return get_from_id(id);
}

LRESULT CALLBACK internal_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_NCCREATE:
    {
        L_DBG(SetLastError(0));
        const window_id id{ windows.add() };
        windows[id].hwnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, id);
        assert(GetLastError() == 0);
        break;
    }
    case WM_DESTROY: get_from_handle(hwnd).closed = true; break;
    case WM_SIZE: resized = (wParam != SIZE_MINIMIZED); break;
    default: break;
    }

    if (resized && GetAsyncKeyState(VK_LBUTTON) >= 0)
    {
        window_info& info = get_from_handle(hwnd);
        assert(info.hwnd);
        GetClientRect(info.hwnd, info.fullscreen ? &info.fullscreen_area : &info.client_area);
        resized = false;
    }

    LONG_PTR longptr = GetWindowLongPtr(hwnd, 0);
    return longptr ? ((window_proc) longptr)(hwnd, msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
}


void resize_window(const window_info& info, const RECT& area)
{
    RECT winrect = area;
    AdjustWindowRect(&winrect, info.style, FALSE);
    const i32 width  = winrect.right - winrect.left;
    const i32 height = winrect.bottom - winrect.top;
    MoveWindow(info.hwnd, info.top_left.x, info.top_left.y, width, height, TRUE);
}

void resize_window(const window_id id, const u32 width, const u32 height)
{
    window_info& info = get_from_id(id);

    if (info.style & WS_CHILD)
    {
        GetClientRect(info.hwnd, &info.client_area);
    } else
    {
        RECT& area  = info.fullscreen ? info.fullscreen_area : info.client_area;
        area.bottom = area.top + height;
        area.right  = area.left + width;
        resize_window(info, area);
    }
}

void set_window_fullscreen(window_id id, bool fullscreen)
{
    window_info& info = get_from_id(id);
    if (info.fullscreen != fullscreen)
    {
        info.fullscreen = fullscreen;
        if (fullscreen)
        {
            GetClientRect(info.hwnd, &info.client_area);
            RECT rect;
            GetWindowRect(info.hwnd, &rect);
            info.top_left.x = rect.left;
            info.top_left.y = rect.top;
            SetWindowLongPtr(info.hwnd, GWL_STYLE, 0);
            ShowWindow(info.hwnd, SW_MAXIMIZE);
        } else
        {
            SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
            resize_window(info, info.client_area);
            ShowWindow(info.hwnd, SW_SHOWNORMAL);
        }
    }
}

bool is_window_fullscreen(const window_id id)
{
    return get_from_id(id).fullscreen;
}

window_handle get_window_handle(const window_id id)
{
    return get_from_id(id).hwnd;
}

void set_window_caption(const window_id id, const wchar_t* caption)
{
    const window_info& info = get_from_id(id);
    SetWindowText(info.hwnd, caption);
}

vec2u get_window_size(const window_id id)
{
    const window_info& info = get_from_id(id);
    const RECT&        area = info.fullscreen ? info.fullscreen_area : info.client_area;

    return { (u32) area.right - (u32) area.left, (u32) area.bottom - (u32) area.top };
}

vec4u get_window_rect(const window_id id)
{
    const window_info& info = get_from_id(id);
    const RECT&        area = info.fullscreen ? info.fullscreen_area : info.client_area;

    return { (u32) area.left, (u32) area.top, (u32) area.right, (u32) area.bottom };
}

bool is_window_closed(const window_id id)
{
    return get_from_id(id).closed;
}

} // namespace

window create_window(const window_create_info* const info)
{
    window_proc   callback = info ? info->callback : nullptr;
    window_handle parent   = info ? info->parent : nullptr;

    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(wc));

    wc.cbSize        = sizeof(WNDCLASSEX);      // Size of the struct
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
    winInfo.client_area.right  = info && info->width ? winInfo.client_area.left + info->width : winInfo.client_area.right;
    winInfo.client_area.bottom = info && info->height ? winInfo.client_area.top + info->height : winInfo.client_area.bottom;

    RECT rect{ winInfo.client_area };

    winInfo.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;

    AdjustWindowRect(&rect, winInfo.style, FALSE);

    const wchar_t* caption = info && info->caption ? info->caption : L"Lotus Game";
    const i32      left    = info ? info->left : winInfo.top_left.x;
    const i32      top     = info ? info->top : winInfo.top_left.y;
    const i32      width   = rect.right - rect.left;
    const i32      height  = rect.bottom - rect.top;


    // extended style, window class name, instance title, window style, x pos, y pos, width, height, menu, hinstance, extra params
    winInfo.hwnd = CreateWindowEx(0, wc.lpszClassName, caption, winInfo.style, left, top, width, height, parent, nullptr,
                                  nullptr, nullptr);

    if (winInfo.hwnd)
    {
        L_DBG(SetLastError(0));

        if (callback)
            SetWindowLongPtr(winInfo.hwnd, 0, (LONG_PTR) callback);
        assert(GetLastError() == 0);

        ShowWindow(winInfo.hwnd, SW_SHOWNORMAL);
        UpdateWindow(winInfo.hwnd);

        window_id id{ (id::id_type) GetWindowLongPtr(winInfo.hwnd, GWLP_USERDATA) };
        windows[id] = winInfo;

        return window(id);
    }

    return {};
}

void remove_window(const window_id id)
{
    const window_info& info = get_from_id(id);
    DestroyWindow(info.hwnd);
    windows.remove(id);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Window Class Implementations ////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void window::set_fullscreen(const bool fullscreen) const
{
    assert(is_valid());
    set_window_fullscreen(m_id, fullscreen);
}

bool window::is_fullscreen() const
{
    assert(is_valid());
    return is_window_fullscreen(m_id);
}

void window::set_caption(const wchar_t* caption) const
{
    assert(is_valid());
    set_window_caption(m_id, caption);
}

vec2u window::size() const
{
    assert(is_valid());
    return get_window_size(m_id);
}

vec4u window::rect() const
{
    assert(is_valid());
    return get_window_rect(m_id);
}

void window::resize(const u32 width, const u32 height) const
{
    assert(is_valid());
    resize_window(m_id, width, height);
}

u32 window::width() const
{
    return size().x;
}

u32 window::height() const
{
    return size().y;
}

bool window::is_closed() const
{
    assert(is_valid());
    return is_window_closed(m_id);
}

void* window::handle() const
{
    assert(is_valid());
    return get_window_handle(m_id);
}

} // namespace lotus::platform
