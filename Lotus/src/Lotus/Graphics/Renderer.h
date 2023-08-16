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
#include "API/Light.h"

namespace lotus::graphics
{

struct frame_info
{
    id::id_type* render_item_ids   = nullptr;
    f32*         thresholds        = nullptr;
    u32          render_item_count = 0;
    camera_id    cam_id{ id::invalid_id };
};

L_TYPED_ID(surface_id)

class surface
{
public:
    constexpr surface() = default;
    constexpr explicit surface(const surface_id id) : m_id(id) {}

    [[nodiscard]] constexpr surface_id get_id() const { return m_id; }

    [[nodiscard]] constexpr bool is_valid() const { return id::is_valid(m_id); }

    void resize(u32 width, u32 height) const;

    [[nodiscard]] u32 width() const;
    [[nodiscard]] u32 height() const;

    void render(frame_info info) const;

private:
    surface_id m_id{ id::invalid_id };
};

struct render_surface
{
    platform::window window{};
    surface          surface{};
};

struct directional_light_params
{};

struct point_light_params
{
    vec3 attenuation;
    f32  range;
};

struct spot_light_params
{
    vec3 attenuation;
    f32  range;
    f32  umbra;    // Umbra angle in radians [0, pi)
    f32  penumbra; // Penumbra angle in radians [umbra, pi)
};

struct light_init_info
{
    u64         light_set_key{ 0 };
    id::id_type entity_id{ id::invalid_id };
    light::type type{};
    f32         intensity{ 1.0f };
    vec3        color{ 1.0f, 1.0f, 1.0f };

    union
    {
        directional_light_params directional_params;
        point_light_params       point_params;
        spot_light_params        spot_params;
    };

    bool is_enabled{ true };
};

struct light_parameter
{
    enum parameter : u32
    {
        is_enabled,
        intensity,
        color,
        type,
        entity_id,

        count
    };
};

struct camera_init_info
{
    id::id_type  entity_id{ id::invalid_id };
    camera::type type{};
    vec3         up{};

    // FoV and aspect ratio is only used with perspective cameras
    // Width and height are for ortho cameras. A union makes sense in this case
    union
    {
        f32 field_of_view{};
        f32 view_width;
    };

    union
    {
        f32 aspect_ratio{};
        f32 view_height;
    };

    f32 near_z{};
    f32 far_z{};
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
        assert(id::is_valid(id));
        entity_id     = id;
        type          = camera::perspective;
        up            = { 0.0f, 1.0f, 0.0f };
        field_of_view = 0.25f;        // 45 degree FOV
        aspect_ratio  = 16.0f / 9.0f; // Will be changed when window(s) are resized
        near_z        = 0.01f;
        far_z         = 1000.0f;
    }
};

struct orthographic_camera_init_info : camera_init_info
{
    explicit orthographic_camera_init_info(id::id_type id)
    {
        assert(id::is_valid(id));
        entity_id   = id;
        type        = camera::orthographic;
        up          = { 0.0f, 1.0f, 0.0f };
        view_width  = 1920.0f; // Because 1920x1080 is the more commonly used
        view_height = 1080.0f; // And is a good size for debugging on my 4k monitor
        near_z      = 0.01f;
        far_z       = 1000.0f;
    }
};

struct shader_flags
{
    enum flags : u32
    {
        none          = 0x0,
        vertex        = 0x01,
        hull          = 0x02,
        domain        = 0x04,
        geometry      = 0x08,
        pixel         = 0x10,
        compute       = 0x20,
        amplification = 0x40,
        mesh          = 0x80,
    };
};

struct shader_type
{
    enum type : u32
    {
        vertex = 0,
        hull,
        domain,
        geometry,
        pixel,
        compute,
        amplification,
        mesh,

        count
    };
};

struct material_type
{
    enum type : u32
    {
        opaque,
        // transparent, unlit, skin, foliage, hair, clear_coat....

        count
    };
};

struct material_init_info
{
    material_type::type type{};
    u32                 texture_count{};
    id::id_type         shader_ids[shader_type::count]{ id::invalid_id, id::invalid_id, id::invalid_id, id::invalid_id,
                                                        id::invalid_id, id::invalid_id, id::invalid_id, id::invalid_id };
    id::id_type*        texture_ids{};
};

struct primitive_topology
{
    enum type : u32
    {
        point_list = 1,
        line_list,
        line_strip,
        triangle_list,
        triangle_strip,


        count
    };
};

enum class graphics_platform : u32
{
    d3d12 = 0,
    //TODO: vulkan = 1
};

bool initialize(graphics_platform platform);

void shutdown();

const char* get_engine_shaders_path();
const char* get_engine_shaders_path(graphics_platform platform);

surface create_surface(platform::window window);
void    remove_surface(surface_id id);

light create_light(light_init_info info);
void  remove_light(light_id id, u64 light_set_key);

camera create_camera(camera_init_info info);
void   remove_camera(camera_id id);


id::id_type add_submesh(const u8*& data);
void        remove_submesh(id::id_type id);

id::id_type add_material(material_init_info info);
void        remove_material(id::id_type id);

id::id_type add_render_item(id::id_type entity_id, id::id_type geometry_content_id, u32 material_count,
                            const id::id_type* const material_ids);

void remove_render_item(id::id_type id);


} // namespace lotus::graphics
