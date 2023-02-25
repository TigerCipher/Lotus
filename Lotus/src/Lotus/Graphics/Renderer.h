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
// File Name: Renderer.h
// Date File Created: 08/29/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "../Core/Common.h"
#include "../Platform/Window.h"
#include "../EngineApi/Camera.h"

namespace lotus::graphics
{

L_TYPED_ID(surface_id)

class surface
{
public:
    constexpr surface() = default;
    constexpr explicit surface(const surface_id id) : m_id(id) {}

    constexpr surface_id get_id() const { return m_id; }

    constexpr bool is_valid() const { return id::is_valid(m_id); }

    void resize(u32 width, u32 height) const;

    u32 width() const;
    u32 height() const;

    void render() const;

private:
    surface_id m_id{ id::invalid_id };
};

struct render_surface
{
    platform::window window{};
    surface          surface{};
};

enum class graphics_platform : u32
{
    d3d12 = 0,
    //TODO: vulkan = 1
};

bool initialize(graphics_platform platform);

void shutdown();

surface create_surface(platform::window window);
void    remove_surface(surface_id id);

const char* get_engine_shaders_path();
const char* get_engine_shaders_path(graphics_platform platform);

id::id_type add_submesh(const u8*& data);
void remove_submesh(id::id_type id);

} // namespace lotus::graphics
