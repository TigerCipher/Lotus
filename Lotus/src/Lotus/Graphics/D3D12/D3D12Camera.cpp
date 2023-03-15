//  ------------------------------------------------------------------------------
//
//  Lotus
//     Copyright 2023 Matthew Rogers
//
//     Licensed under the Apache License, Version 2.0 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
//
//         http://www.apache.org/licenses/LICENSE-2.0
//
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.
//
//  File Name: D3D12Camera.h
//  Date File Created: 02/25/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#include "D3D12Camera.h"
#include "API/GameEntity.h"

namespace lotus::graphics::d3d12::camera
{

namespace
{
utl::free_list<d3d12_camera> cameras;

void set_up_vector(d3d12_camera camera, const void* const data, u32 size)
{
    vec3 up_vector = *(vec3*) data;
    assert(sizeof(up_vector) == size);
    camera.up(up_vector);
}

void set_field_of_view(d3d12_camera camera, const void* const data, u32 size)
{
    assert(camera.projection_type() == graphics::camera::perspective);
    f32 fov = *(f32*) data;
    assert(sizeof(fov) == size);
    camera.field_of_view(fov);
}

void set_aspect_ratio(d3d12_camera camera, const void* const data, u32 size)
{
    assert(camera.projection_type() == graphics::camera::perspective);
    f32 aspect_ratio = *(f32*) data;
    assert(sizeof(aspect_ratio) == size);
    camera.aspect_ratio(aspect_ratio);
}

void set_view_width(d3d12_camera camera, const void* const data, u32 size)
{
    assert(camera.projection_type() == graphics::camera::orthographic);
    f32 view_width = *(f32*) data;
    assert(sizeof(view_width) == size);
    camera.view_width(view_width);
}

void set_view_height(d3d12_camera camera, const void* const data, u32 size)
{
    assert(camera.projection_type() == graphics::camera::orthographic);
    f32 view_height = *(f32*) data;
    assert(sizeof(view_height) == size);
    camera.view_height(view_height);
}

void set_near_z(d3d12_camera camera, const void* const data, u32 size)
{
    f32 near_z = *(f32*) data;
    assert(sizeof(near_z) == size);
    camera.near_z(near_z);
}

void set_far_z(d3d12_camera camera, const void* const data, u32 size)
{
    f32 far_z = *(f32*) data;
    assert(sizeof(far_z) == size);
    camera.far_z(far_z);
}

void get_view(d3d12_camera camera, void* const data, u32 size)
{
    mat4* const matrix = (mat4* const) data;
    assert(sizeof(mat4) == size);
    math::store_float4x4(matrix, camera.view());
}

void get_projection(d3d12_camera camera, void* const data, u32 size)
{
    mat4* const matrix = (mat4* const) data;
    assert(sizeof(mat4) == size);
    math::store_float4x4(matrix, camera.projection());
}

void get_inverse_projection(d3d12_camera camera, void* const data, u32 size)
{
    mat4* const matrix = (mat4* const) data;
    assert(sizeof(mat4) == size);
    math::store_float4x4(matrix, camera.inverse_projection());
}

void get_view_projection(d3d12_camera camera, void* const data, u32 size)
{
    mat4* const matrix = (mat4* const) data;
    assert(sizeof(mat4) == size);
    math::store_float4x4(matrix, camera.view_projection());
}

void get_inverse_view_projection(d3d12_camera camera, void* const data, u32 size)
{
    mat4* const matrix = (mat4* const) data;
    assert(sizeof(mat4) == size);
    math::store_float4x4(matrix, camera.inverse_view_projection());
}

void get_up_vector(d3d12_camera camera, void* const data, u32 size)
{
    vec3* const up_vector = (vec3* const) data;
    assert(sizeof(vec3) == size);
    math::store_float3(up_vector, camera.up());
}

void get_field_of_view(d3d12_camera camera, void* const data, u32 size)
{
    assert(camera.projection_type() == graphics::camera::perspective);
    f32* const fov = (f32* const) data;
    assert(sizeof(f32) == size);
    *fov = camera.field_of_view();
}

void get_aspect_ratio(d3d12_camera camera, void* const data, u32 size)
{
    assert(camera.projection_type() == graphics::camera::perspective);
    f32* const aspect_ratio = (f32* const) data;
    assert(sizeof(f32) == size);
    *aspect_ratio = camera.aspect_ratio();
}

void get_view_width(d3d12_camera camera, void* const data, u32 size)
{
    assert(camera.projection_type() == graphics::camera::orthographic);
    f32* const view_width = (f32* const) data;
    assert(sizeof(f32) == size);
    *view_width = camera.view_width();
}

void get_view_height(d3d12_camera camera, void* const data, u32 size)
{
    assert(camera.projection_type() == graphics::camera::orthographic);
    f32* const view_height = (f32* const) data;
    assert(sizeof(f32) == size);
    *view_height = camera.view_height();
}

void get_near_z(d3d12_camera camera, void* const data, u32 size)
{
    f32* const near_z = (f32* const) data;
    assert(sizeof(f32) == size);
    *near_z = camera.near_z();
}

void get_far_z(d3d12_camera camera, void* const data, u32 size)
{
    f32* const far_z = (f32* const) data;
    assert(sizeof(f32) == size);
    *far_z = camera.far_z();
}

void get_projection_type(d3d12_camera camera, void* const data, u32 size)
{
    graphics::camera::type* const type = (graphics::camera::type* const) data;
    assert(sizeof(graphics::camera::type) == size);
    *type = camera.projection_type();
}

void get_entity_id(d3d12_camera camera, void* const data, u32 size)
{
    id::id_type* const entity_id = (id::id_type* const) data;
    assert(sizeof(id::id_type) == size);
    *entity_id = camera.entity_id();
}

void dummy_set(d3d12_camera, const void* const, u32) {}

using set_function = void (*)(d3d12_camera, const void* const, u32);
using get_function = void (*)(d3d12_camera, void* const, u32);

constexpr set_function setters[]{
    set_up_vector, set_field_of_view, set_aspect_ratio, set_view_width, set_view_height, set_near_z, set_far_z,
    dummy_set,     dummy_set,         dummy_set,        dummy_set,      dummy_set,       dummy_set,  dummy_set,
};

static_assert(_countof(setters) == camera_parameter::count);

constexpr get_function getters[]{
    get_up_vector,       get_field_of_view,
    get_aspect_ratio,    get_view_width,
    get_view_height,     get_near_z,
    get_far_z,           get_view,
    get_projection,      get_inverse_projection,
    get_view_projection, get_inverse_view_projection,
    get_projection_type, get_entity_id,
};

static_assert(_countof(getters) == camera_parameter::count);

} // anonymous namespace

d3d12_camera::d3d12_camera(camera_init_info info) :
    m_up(math::load_float3(&info.up)),
    m_fov(info.field_of_view),
    m_aspect_ratio(info.aspect_ratio),
    m_near_z(info.near_z),
    m_far_z(info.far_z),
    m_type(info.type),
    m_entity_id(info.entity_id),
    m_is_dirty(true)
{
    assert(id::is_valid(m_entity_id));
    update();
}

void d3d12_camera::update()
{
    game_entity::entity entity(game_entity::entity_id{m_entity_id});
    vec3 pos = entity.transform().position();
    vec3 dir = entity.transform().orientation();

    vec position = math::load_float3(&pos);
    vec direction = math::load_float3(&dir);
    m_view = math::look_to_rh(position, direction, m_up);

    if(m_is_dirty)
    {
        m_projection = m_type == graphics::camera::perspective ? math::perspective_fov_rh(m_fov * dx_pi, m_aspect_ratio, m_near_z, m_far_z) :
        math::orthographic_rh(m_view_width, m_view_height, m_near_z, m_far_z);

        m_inverse_projection = math::inverse_matrix(nullptr, m_projection);

        m_is_dirty = false;
    }

    m_view_projection = math::mul_matrix(m_view, m_projection);
    m_inverse_view_projection = math::inverse_matrix(nullptr, m_view_projection);
}

void d3d12_camera::up(vec3 up)
{
    m_up       = math::load_float3(&up);
    m_is_dirty = true;
}

void d3d12_camera::field_of_view(f32 fov)
{
    assert(m_type == graphics::camera::perspective);
    m_fov      = fov;
    m_is_dirty = true;
}

void d3d12_camera::aspect_ratio(f32 ratio)
{
    assert(m_type == graphics::camera::perspective);
    m_aspect_ratio = ratio;
    m_is_dirty     = true;
}

void d3d12_camera::view_width(f32 width)
{
    assert(width);
    assert(m_type == graphics::camera::orthographic);
    m_view_width = width;
    m_is_dirty   = true;
}

void d3d12_camera::view_height(f32 height)
{
    assert(height);
    assert(m_type == graphics::camera::orthographic);
    m_view_height = height;
    m_is_dirty    = true;
}

void d3d12_camera::near_z(f32 near_z)
{
    m_near_z   = near_z;
    m_is_dirty = true;
}

void d3d12_camera::far_z(f32 far_z)
{
    m_far_z    = far_z;
    m_is_dirty = true;
}

graphics::camera create(camera_init_info info)
{
    return graphics::camera(camera_id{ cameras.add(info) });
}

void remove(camera_id id)
{
    assert(id::is_valid(id));
    cameras.remove(id);
}

void set_parameter(camera_id id, camera_parameter::parameter param, const void* const data, u32 data_size)
{
    assert(data && data_size);
    assert(param < camera_parameter::count);
    d3d12_camera& camera = get(id);
    setters[param](camera, data, data_size);
}

void get_parameter(camera_id id, camera_parameter::parameter param, void* const data, u32 data_size)
{
    assert(data && data_size);
    assert(param < camera_parameter::count);
    d3d12_camera& camera = get(id);
    getters[param](camera, data, data_size);
}

d3d12_camera& get(camera_id id)
{
    assert(id::is_valid(id));
    return cameras[id];
}

} // namespace lotus::graphics::d3d12::camera
