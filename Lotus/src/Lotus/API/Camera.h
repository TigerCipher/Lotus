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
//  File Name: Camera.h
//  Date File Created: 02/25/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#pragma once

#include "../Common.h"

namespace lotus::graphics
{
L_TYPED_ID(camera_id);

class camera
{
public:
    enum type : u32
    {
        perspective,
        orthographic,
    };

    constexpr camera() = default;
    constexpr explicit camera(const camera_id id) : m_id(id) {}

    constexpr camera_id get_id() const { return m_id; }
    constexpr bool      is_valid() const { return id::is_valid(m_id); }

    void up(vec3 up) const;
    void field_of_view(f32 fov) const;
    void aspect_ratio(f32 ratio) const;
    void view_width(f32 width) const;
    void view_height(f32 height) const;
    void range(f32 near_z, f32 far_z) const;

    mat4 view() const;
    mat4 projection() const;
    mat4 inverse_projection() const;
    mat4 view_projection() const;
    mat4 inverse_view_projection() const;

    vec3 up() const;
    f32  field_of_view() const;
    f32  aspect_ratio() const;
    f32  view_width() const;
    f32  view_height() const;
    f32  near_z() const;
    f32  far_z() const;

    type        projection_type() const;
    id::id_type entity_id() const;

private:
    camera_id m_id{ id::invalid_id };
};
} // namespace lotus::graphics