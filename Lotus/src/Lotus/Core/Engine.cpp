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
// File Name: Engine.cpp
// Date File Created: 8/26/2022
// Author: Matt
//
// ------------------------------------------------------------------------------

#ifndef PRODUCTION

    #include "Content/ContentLoader.h"
    #include "Components/Script.h"
    #include "Platform/Platform.h"
    #include "Graphics/Renderer.h"

    #include <thread>
using namespace lotus;
namespace
{

graphics::render_surface game_window{};

LRESULT winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
    {
        if (game_window.window.is_closed())
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    break;

    case WM_SYSCHAR:
        if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN))
        {
            game_window.window.set_fullscreen(!game_window.window.is_fullscreen());
            return 0;
        }
        break;

    default: break;
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}
} // namespace


bool engine_initialize()
{
    LOG_INFO("Initializing Lotus engine");
    LOG_INFO("Loading game");
    if (!content::load_game())
        return false;

    constexpr platform::window_create_info info{ &winproc, nullptr, L"Lotus Game" };
    LOG_INFO("Setting up platform");
    game_window.window = platform::create_window(&info);

    if (!game_window.window.is_valid())
    {
        LOG_ERROR("Failed to create window");
        return false;
    }

    return true;
}
void engine_update()
{
    lotus::script::update_all(10.0f);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void engine_shutdown()
{
    LOG_INFO("Shutting down Lotus engine");
    platform::remove_window(game_window.window.get_id());
    LOG_INFO("Unloading game");
    content::unload_game();
}

#endif
