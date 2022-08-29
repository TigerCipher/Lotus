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
// File Name: WindowTest.h
// Date File Created: 8/29/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "Test.h"
#include "Lotus/Platform/Platform.h"

using namespace lotus;

constexpr u32    numWindows = 4;
platform::Window windows [ numWindows ];


LRESULT winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
    {
        bool all = true;
        for (auto& window : windows)
        {
            if (!window.IsClosed()) all = false;
        }

        if (all)
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    break;

    case WM_SYSCHAR:
        if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN))
        {
            platform::Window win(platform::window_id { (id::id_type) GetWindowLongPtr(hwnd, GWLP_USERDATA) });
            win.SetFullscreen(!win.IsFullscreen());
            return 0;
        }
        break;

    default: break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

class EngineTest : public Test
{
public:
    bool Init() override
    {
        platform::window_create_info info [ numWindows ] {
            { &winproc, nullptr, L"Test Window 1", 100, 100, 400, 800 },
            { &winproc, nullptr, L"Test Window 2", 200, 200, 400, 400 },
            { &winproc, nullptr, L"Test Window 3", 300, 300, 800, 400 },
            { &winproc, nullptr, L"Test Window 4", 400, 400, 800, 600 },
        };

        for (u32 i = 0; i < numWindows; ++i)
        {
            windows [ i ] = platform::create_window(&info [ i ]);
            int w         = windows [ i ].Width();
            int a         = 0;
        }

        return true;
    }
    void Run() override { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }

    void Shutdown() override
    {
        for (auto& window : windows) { platform::remove_window(window.GetId()); }
    }
};
