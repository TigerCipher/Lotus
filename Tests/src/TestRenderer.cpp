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
#include "ShaderCompiler.h"

#include "Lotus/Platform/Platform.h"
#include "Lotus/Graphics/Renderer.h"
#if TEST_RENDERER

using namespace lotus;

constexpr u32 numWindows = 4;

timer timer;

graphics::render_surface surfaces[numWindows];

bool is_restarting = false;

void destroy_render_surface(graphics::render_surface& surface);
bool test_initialize();
void test_shutdown();

LRESULT winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
    {
        bool all = true;
        for (auto& s : surfaces)
        {
            if (s.window.is_valid())
            {
                if (s.window.is_closed())
                {
                    destroy_render_surface(s);
                } else
                {
                    all = false;
                }
            }
        }

        if (all && !is_restarting)
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
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        } else if (wparam == VK_F11)
        {
            is_restarting = true;
            test_shutdown();
            test_initialize();
        }

    default: break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void create_render_surface(graphics::render_surface& surface, platform::window_create_info info)
{
    surface.window  = platform::create_window(&info);
    surface.surface = graphics::create_surface(surface.window);
}

void destroy_render_surface(graphics::render_surface& surface)
{
    graphics::render_surface temp{ surface };
    surface = {};
    if (temp.surface.is_valid())
        graphics::remove_surface(temp.surface.get_id());
    if (temp.window.is_valid())
        platform::remove_window(temp.window.get_id());
}

bool test_initialize()
{
    while (!compile_shaders())
    {
        if (MessageBox(nullptr, L"Failed to compile engine shaders", L"Shader Compilation Error", MB_RETRYCANCEL) !=
            IDRETRY)
            return false;
    }

    if (!graphics::initialize(graphics::graphics_platform::d3d12))
        return false;

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

    is_restarting = false;
    return true;
}

void test_shutdown()
{
    for (auto& s : surfaces)
    {
        destroy_render_surface(s);
    }

    graphics::shutdown();
}

bool EngineTest::Init()
{
    return test_initialize();
}
void EngineTest::Run()
{
    timer.begin();
    // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    for (u32 i = 0; i < numWindows; ++i)
    {
        if (surfaces[i].surface.is_valid())
        {
            surfaces[i].surface.render();
        }
    }
    timer.end();
}

void EngineTest::Shutdown()
{
    test_shutdown();
}

#endif