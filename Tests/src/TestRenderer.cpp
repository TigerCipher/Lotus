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

#include <filesystem>
#include <fstream>

#include "ShaderCompiler.h"

#include "Lotus/Platform/Platform.h"
#include "Lotus/Graphics/Renderer.h"
#include "Lotus/Graphics/D3D12/D3D12Core.h"

#include "Lotus/Content/ContentToEngine.h"

#if TEST_RENDERER

using namespace lotus;

    // Multithreading
    #define ENABLE_TEST_WORKERS 0
constexpr u32 num_threads     = 8;
bool          should_shutdown = false;
std::thread   workers[num_threads];

utl::vector<u8> buffer(1024 * 1024, 0);

void buffer_test_worker()
{
    while(!should_shutdown)
    {
        auto* res = graphics::d3d12::d3dx::create_buffer(buffer.data(), (u32)buffer.size());
        graphics::d3d12::core::deferred_release(res);
    }
}


template<class Func, class... Args>
void init_test_workers(Func&& fn, Args&&... args)
{
    #if ENABLE_TEST_WORKERS
    should_shutdown = false;
    for (auto& w : workers)
    {
        w = std::thread(std::forward<Func>(fn), std::forward<Args>(args)...);
    }
    #endif
}

void join_test_workers()
{
    #if ENABLE_TEST_WORKERS
    should_shutdown = true;
    for (auto& w : workers)
    {
        w.join();
    }
    #endif
}

/////////////////////


constexpr u32 numWindows = 4;

timer_lt timer;

graphics::render_surface surfaces[numWindows];

id::id_type model_id = id::invalid_id;

bool is_restarting = false;
bool resized       = false;

void destroy_render_surface(graphics::render_surface& surface);
bool test_initialize();
void test_shutdown();

LRESULT winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    bool toggle_fullscreen = false;

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
    case WM_SIZE: resized = (wparam != SIZE_MINIMIZED); break;
    case WM_SYSCHAR:
        toggle_fullscreen = (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN));
        if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN))
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

    if ((resized && GetAsyncKeyState(VK_LBUTTON) >= 0) || toggle_fullscreen)
    {
        platform::window win{ platform::window_id{ (id::id_type) GetWindowLongPtr(hwnd, GWLP_USERDATA) } };
        for (u32 i = 0; i < _countof(surfaces); ++i)
        {
            if (win.get_id() == surfaces[i].window.get_id())
            {
                if (toggle_fullscreen)
                {
                    win.set_fullscreen(!win.is_fullscreen());
                    return 0;
                } else
                {
                    surfaces[i].surface.resize(win.width(), win.height());

                    resized = false;
                }
                break;
            }
        }
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

bool read_file(std::filesystem::path path, scope<u8[]>& data, u64& size)
{
    if (!std::filesystem::exists(path))
        return false;
    size = std::filesystem::file_size(path);
    LASSERT(size);
    if (!size)
        return false;
    data = create_scope<u8[]>(size);
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file || !file.read((char*) data.get(), size))
    {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool test_initialize()
{
    while (!compile_shaders())
    {
        if (MessageBox(nullptr, L"Failed to compile engine shaders", L"Shader Compilation Error", MB_RETRYCANCEL) != IDRETRY)
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

    scope<u8[]> model;
    u64 size = 0;
    if(!read_file(R"(..\..\Tests\model.model)", model, size)) return false;
    model_id = content::create_resource(model.get(), content::asset_type::mesh);
    if(!id::is_valid(model_id)) return false;

    init_test_workers(buffer_test_worker);

    is_restarting = false;
    return true;
}

void test_shutdown()
{
    join_test_workers();

    if(id::is_valid(model_id))
    {
        content::destroy_resource(model_id, content::asset_type::mesh);
    }

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
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
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