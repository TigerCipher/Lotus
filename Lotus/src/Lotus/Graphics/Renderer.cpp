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
// File Name: Renderer.cpp
// Date File Created: 10/30/2022
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "Renderer.h"

#include "GraphicsPlatformInterface.h"
#include "D3D12/D3D12Interface.h"

namespace lotus::graphics
{
namespace
{
platform_interface gfx{};

constexpr const char* engine_shader_paths[]{
    R"(.\shaders\d3d12\shaders.bin)",
    // ".\\shaders\\vulkan\\shaders.bin",
};

bool set_platform_interface(graphics_platform platform)
{
    switch (platform)
    {
    case graphics_platform::d3d12: d3d12::get_platform_interface(gfx); break;
    default: return false;
    }

    LASSERT(gfx.platform == platform);
    return true;
}
} // namespace

void surface::resize(u32 width, u32 height) const
{
    LASSERT(is_valid());
    gfx.surface.resize(m_id, width, height);
}

u32 surface::width() const
{
    LASSERT(is_valid());
    return gfx.surface.width(m_id);
}

u32 surface::height() const
{
    LASSERT(is_valid());
    return gfx.surface.height(m_id);
}

void surface::render() const
{
    LASSERT(is_valid());
    gfx.surface.render(m_id);
}

bool initialize(graphics_platform platform)
{
    return set_platform_interface(platform) && gfx.initialize();
}

void shutdown()
{
    if(gfx.platform != (graphics_platform) -1)
        gfx.shutdown();
}


surface create_surface(platform::window window)
{
    return gfx.surface.create(window);
}

void remove_surface(surface_id id)
{
    LASSERT(id::is_valid(id));
    gfx.surface.remove(id);
}

const char* get_engine_shaders_path()
{
    return engine_shader_paths[(u32) gfx.platform];
}

const char* get_engine_shaders_path(graphics_platform platform)
{
    return engine_shader_paths[(u32) platform];
}
} // namespace lotus::graphics