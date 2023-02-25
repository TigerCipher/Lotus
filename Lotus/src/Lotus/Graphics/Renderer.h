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

#include "Common.h"
#include "Platform/Window.h"
#include "API/Camera.h"

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

struct camera_init_info
{
    id::id_type  entity_id{ id::invalid_id };
    camera::type type{};
    vec3         up;

    // FoV and aspect ratio is only used with perspective cameras
    // Width and height are for ortho cameras. A union makes sense in this case
    union
    {
        f32 field_of_view;
        f32 view_width;
    };

    union
    {
        f32 aspect_ratio;
        f32 view_height;
    };

    f32 near_z;
    f32 far_z;
};

struct camera_parameter
{
    enum parameter : u32
    {
        up_vector,
        field_of_view,
        aspect_ratio,
        view_width,
        view_height,
        near_z,
        far_z,
        view,
        projection,
        inverse_projection,
        view_projection,
        inverse_view_projection,
        type,
        entity_id,

        count
    };
};

struct perspective_camera_init_info : camera_init_info
{
    explicit perspective_camera_init_info(id::id_type id)
    {
        LASSERT(id::is_valid(id));
        entity_id     = id;
        type          = camera::perspective;
        up            = { 0.0f, 1.0f, 0.0f };
        field_of_view = 0.25f;        // 45 degree FOV
        aspect_ratio  = 16.0f / 9.0f; // Will be changed when window(s) are resized
        near_z        = 0.001f;
        far_z         = 10000.0f;
    }
};

struct orthographic_camera_init_info : camera_init_info
{
    explicit orthographic_camera_init_info(id::id_type id)
    {
        LASSERT(id::is_valid(id));
        entity_id   = id;
        type        = camera::orthographic;
        up          = { 0.0f, 1.0f, 0.0f };
        view_width  = 1920.0f; // Because 1920x1080 is the more commonly used
        view_height = 1080.0f; // And is a good size for debugging on my 4k monitor
        near_z      = 0.001f;
        far_z       = 10000.0f;
    }
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
void        remove_submesh(id::id_type id);

camera create_camera(camera_init_info info);
void   remove_camera(camera_id id);

} // namespace lotus::graphics
