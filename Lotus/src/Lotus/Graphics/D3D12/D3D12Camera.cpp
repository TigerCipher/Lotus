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
{} // anonymous namespace

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
    LASSERT(id::is_valid(m_entity_id));
    update();
}

void d3d12_camera::update() {}

void d3d12_camera::up(vec3 up)
{
    m_up = math::load_float3(&up);
    m_is_dirty = true;
}

void d3d12_camera::field_of_view(f32 fov)
{
    LASSERT(m_type == graphics::camera::perspective);
    m_fov = fov;
    m_is_dirty = true;
}

void d3d12_camera::aspect_ratio(f32 ratio)
{
    LASSERT(m_type == graphics::camera::perspective);
    m_aspect_ratio = ratio;
    m_is_dirty     = true;
}

void d3d12_camera::view_width(f32 width)
{
    LASSERT(width);
    LASSERT(m_type == graphics::camera::orthographic);
    m_view_width = width;
    m_is_dirty   = true;
}

void d3d12_camera::view_height(f32 height)
{
    LASSERT(height);
    LASSERT(m_type == graphics::camera::orthographic);
    m_view_height = height;
    m_is_dirty    = true;
}

void d3d12_camera::near_z(f32 near_z)
{
    m_near_z = near_z;
    m_is_dirty = true;
}

void d3d12_camera::far_z(f32 far_z)
{
    m_far_z = far_z;
    m_is_dirty = true;
}

} // namespace lotus::graphics::d3d12::camera
