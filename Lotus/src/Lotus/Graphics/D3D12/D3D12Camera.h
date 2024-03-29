﻿//  ------------------------------------------------------------------------------
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

#pragma once

#include "D3D12Common.h"

namespace lotus::graphics::d3d12::camera
{

class d3d12_camera
{
public:
    explicit d3d12_camera(camera_init_info info);

    void update();

    void up(vec3 up);
    constexpr void field_of_view(f32 fov);
    constexpr void aspect_ratio(f32 ratio);
    constexpr void view_width(f32 width);
    constexpr void view_height(f32 height);
    constexpr void near_z(f32 near_z);
    constexpr void far_z(f32 far_z);

    [[nodiscard]] constexpr mat view() const { return m_view; }
    [[nodiscard]] constexpr mat projection() const { return m_projection; }
    [[nodiscard]] constexpr mat inverse_projection() const { return m_inverse_projection; }
    [[nodiscard]] constexpr mat view_projection() const { return m_view_projection; }
    [[nodiscard]] constexpr mat inverse_view_projection() const { return m_inverse_view_projection; }

    [[nodiscard]] constexpr vec up() const { return m_up; }
    [[nodiscard]] constexpr vec position() const { return m_position; }
    [[nodiscard]] constexpr vec direction() const { return m_direction; }
    [[nodiscard]] constexpr f32 field_of_view() const { return m_fov; }
    [[nodiscard]] constexpr f32 aspect_ratio() const { return m_aspect_ratio; }
    [[nodiscard]] constexpr f32 view_width() const { return m_view_width; }
    [[nodiscard]] constexpr f32 view_height() const { return m_view_height; }
    [[nodiscard]] constexpr f32 near_z() const { return m_near_z; }
    [[nodiscard]] constexpr f32 far_z() const { return m_far_z; }

    [[nodiscard]] constexpr graphics::camera::type projection_type() const { return m_type; }
    [[nodiscard]] constexpr id::id_type            entity_id() const { return m_entity_id; }

private:
    mat m_view;
    mat m_projection;
    mat m_inverse_projection;
    mat m_view_projection;
    mat m_inverse_view_projection;

    vec m_up;
    vec m_position;
    vec m_direction;

    union
    {
        f32 m_fov;
        f32 m_view_width;
    };

    union
    {
        f32 m_aspect_ratio;
        f32 m_view_height;
    };

    f32 m_near_z;
    f32 m_far_z;

    graphics::camera::type m_type;
    id::id_type            m_entity_id;

    bool m_is_dirty;
};

graphics::camera create(camera_init_info info);
void             remove(camera_id id);
void             set_parameter(camera_id id, camera_parameter::parameter param, const void* const data, u32 data_size);
void             get_parameter(camera_id id, camera_parameter::parameter param, void* const data, u32 data_size);

[[nodiscard]] d3d12_camera& get(camera_id id);

} // namespace lotus::graphics::d3d12::camera
