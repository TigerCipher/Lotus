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
// File Name: TestRenderer.cpp
// Date File Created: 10/30/2022
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "TestRenderer.h"

#include "Lotus/Platform/Platform.h"
#include "Lotus/Graphics/Renderer.h"
#if TEST_RENDERER

using namespace lotus;

constexpr u32 numWindows = 4;

graphics::render_surface surfaces[numWindows];

LRESULT winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
    {
        bool all = true;
        for (auto& s : surfaces)
        {
            if (!s.window.is_closed())
                all = false;
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
            platform::window win(platform::window_id{ (id::id_type) GetWindowLongPtr(hwnd, GWLP_USERDATA) });
            win.set_fullscreen(!win.is_fullscreen());
            return 0;
        }
        break;

    default: break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void create_render_surface(graphics::render_surface& surface, platform::window_create_info info)
{
    surface.window = platform::create_window(&info);
}

void destroy_render_surface(graphics::render_surface& surface)
{
    platform::remove_window(surface.window.get_id());
}

bool EngineTest::Init()
{
    bool result = graphics::initialize(graphics::graphics_platform::d3d12);
    if (!result)
        return result;

    platform::window_create_info info[numWindows]{
        { &winproc, nullptr, L"Test Window 1", 100, 100, 400, 800 },
        { &winproc, nullptr, L"Test Window 2", 200, 200, 400, 400 },
        { &winproc, nullptr, L"Test Window 3", 300, 300, 800, 400 },
        { &winproc, nullptr, L"Test Window 4", 400, 400, 800, 600 },
    };

    static_assert(_countof(info) == _countof(surfaces));

    for (u32 i = 0; i < numWindows; ++i)
    {
        create_render_surface(surfaces[i], info[i]);
    }

    return result;
}
void EngineTest::Run()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    graphics::render();
}

void EngineTest::Shutdown()
{
    for (auto& s : surfaces)
    {
        destroy_render_surface(s);
    }

    graphics::shutdown();
}

#endif