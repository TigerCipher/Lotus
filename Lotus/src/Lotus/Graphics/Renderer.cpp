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
};

bool set_platform_interface(graphics_platform platform, platform_interface& pinterface)
{
    switch (platform)
    {
    case graphics_platform::d3d12: d3d12::get_platform_interface(pinterface); break;
    default: return false;
    }

    LASSERT(pinterface.platform == platform);
    return true;
}

} // anonymous namespace

bool initialize(graphics_platform platform)
{
    return set_platform_interface(platform, gfx) && gfx.initialize();
}

void shutdown()
{
    if (gfx.platform != (graphics_platform) -1)
        gfx.shutdown();
}

const char* get_engine_shaders_path()
{
    return engine_shader_paths[(u32) gfx.platform];
}

const char* get_engine_shaders_path(graphics_platform platform)
{
    return engine_shader_paths[(u32) platform];
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


camera create_camera(camera_init_info info)
{
    return gfx.camera.create(info);
}

void remove_camera(camera_id id)
{
    gfx.camera.remove(id);
}


id::id_type add_submesh(const u8*& data)
{
    return gfx.resources.add_submesh(data);
}

void remove_submesh(id::id_type id)
{
    gfx.resources.remove_submesh(id);
}

id::id_type add_material(material_init_info info)
{
    return gfx.resources.add_material(info);
}

void remove_material(id::id_type id)
{
    gfx.resources.remove_material(id);
}

id::id_type add_render_item(id::id_type entity_id, id::id_type geometry_content_id, u32 material_count,
                            const id::id_type* const material_ids)
{
    return gfx.resources.add_render_item(entity_id, geometry_content_id, material_count, material_ids);
}


void remove_render_item(id::id_type id)
{
    gfx.resources.remove_render_item(id);
}


///////////////////////////////////////////// Camera Class

void camera::up(vec3 up) const
{
    LASSERT(is_valid());
    gfx.camera.set_parameter(m_id, camera_parameter::up_vector, &up, sizeof(up));
}

void camera::field_of_view(f32 fov) const
{
    LASSERT(is_valid());
    gfx.camera.set_parameter(m_id, camera_parameter::field_of_view, &fov, sizeof(fov));
}

void camera::aspect_ratio(f32 ratio) const
{
    LASSERT(is_valid());
    gfx.camera.set_parameter(m_id, camera_parameter::aspect_ratio, &ratio, sizeof(ratio));
}

void camera::view_width(f32 width) const
{
    LASSERT(is_valid());
    gfx.camera.set_parameter(m_id, camera_parameter::view_width, &width, sizeof(width));
}

void camera::view_height(f32 height) const
{
    LASSERT(is_valid());
    gfx.camera.set_parameter(m_id, camera_parameter::view_height, &height, sizeof(height));
}

void camera::range(f32 near_z, f32 far_z) const
{
    LASSERT(is_valid());
    gfx.camera.set_parameter(m_id, camera_parameter::near_z, &near_z, sizeof(near_z));
    gfx.camera.set_parameter(m_id, camera_parameter::far_z, &far_z, sizeof(far_z));
}

mat4 camera::view() const
{
    LASSERT(is_valid());
    mat4 matrix;
    gfx.camera.get_parameter(m_id, camera_parameter::view, &matrix, sizeof(matrix));
    return matrix;
}

mat4 camera::projection() const
{
    LASSERT(is_valid());
    mat4 matrix;
    gfx.camera.get_parameter(m_id, camera_parameter::projection, &matrix, sizeof(matrix));
    return matrix;
}

mat4 camera::inverse_projection() const
{
    LASSERT(is_valid());
    mat4 matrix;
    gfx.camera.get_parameter(m_id, camera_parameter::inverse_projection, &matrix, sizeof(matrix));
    return matrix;
}

mat4 camera::view_projection() const
{
    LASSERT(is_valid());
    mat4 matrix;
    gfx.camera.get_parameter(m_id, camera_parameter::view_projection, &matrix, sizeof(matrix));
    return matrix;
}

mat4 camera::inverse_view_projection() const
{
    LASSERT(is_valid());
    mat4 matrix;
    gfx.camera.get_parameter(m_id, camera_parameter::inverse_view_projection, &matrix, sizeof(matrix));
    return matrix;
}

vec3 camera::up() const
{
    LASSERT(is_valid());
    vec3 v;
    gfx.camera.get_parameter(m_id, camera_parameter::up_vector, &v, sizeof(v));
    return v;
}

f32 camera::field_of_view() const
{
    LASSERT(is_valid());
    f32 f;
    gfx.camera.get_parameter(m_id, camera_parameter::field_of_view, &f, sizeof(f));
    return f;
}

f32 camera::aspect_ratio() const
{
    LASSERT(is_valid());
    f32 f;
    gfx.camera.get_parameter(m_id, camera_parameter::aspect_ratio, &f, sizeof(f));
    return f;
}

f32 camera::view_width() const
{
    LASSERT(is_valid());
    f32 f;
    gfx.camera.get_parameter(m_id, camera_parameter::view_width, &f, sizeof(f));
    return f;
}

f32 camera::view_height() const
{
    LASSERT(is_valid());
    f32 f;
    gfx.camera.get_parameter(m_id, camera_parameter::view_height, &f, sizeof(f));
    return f;
}

f32 camera::near_z() const
{
    LASSERT(is_valid());
    f32 f;
    gfx.camera.get_parameter(m_id, camera_parameter::near_z, &f, sizeof(f));
    return f;
}

f32 camera::far_z() const
{
    LASSERT(is_valid());
    f32 f;
    gfx.camera.get_parameter(m_id, camera_parameter::far_z, &f, sizeof(f));
    return f;
}

camera::type camera::projection_type() const
{
    LASSERT(is_valid());
    type t;
    gfx.camera.get_parameter(m_id, camera_parameter::type, &t, sizeof(t));
    return t;
}

id::id_type camera::entity_id() const
{
    LASSERT(is_valid());
    id::id_type id;
    gfx.camera.get_parameter(m_id, camera_parameter::entity_id, &id, sizeof(id));
    return id;
}

///////////////////////////////////////////// End of Camera Class


} // namespace lotus::graphics