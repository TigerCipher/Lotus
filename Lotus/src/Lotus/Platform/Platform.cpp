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

    LRESULT CALLBACK internal_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        LONG_PTR longptr = GetWindowLongPtr(hwnd, 0);
        return longptr ? ((window_proc) longptr)(hwnd, msg, wParam, lParam) : DefWindowProc(hwnd, msg, wParam, lParam);
    }
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
    wc.hbrBackground = CreateSolidBrush(RGB(26, 48, 76));
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = L"LotusWindow";
    wc.hIconSm       = LoadIcon(nullptr, IDI_APPLICATION);

    RegisterClassEx(&wc);

    window_info winInfo;
    RECT        rc { winInfo.clientArea };

    AdjustWindowRect(&rc, winInfo.style, FALSE);

    const wchar_t* caption = info && info->caption ? info->caption : L"Lotus Game";
    const s32      left    = info && info->left ? info->left : winInfo.clientArea.left;
    const s32      top     = info && info->top ? info->top : winInfo.clientArea.top;
    const s32      width   = info && info->width ? info->width : rc.right - rc.left;
    const s32      height  = info && info->height ? info->height : rc.bottom - rc.top;

    winInfo.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;

    // extended style, window class name, instance title, window style, x pos, y pos, width, height, menu, hinstance, extra params
    winInfo.hwnd = CreateWindowEx(0, wc.lpszClassName, caption, winInfo.style, left, top, width, height, parent, nullptr,
                               nullptr, nullptr);

    if (winInfo.hwnd)
    {
        // TODO
    }

    return {};
}

void remove_window(window_id id) { }
} // namespace lotus::platform
