// ------------------------------------------------------------------------------
//
// Lotus
//    Copyright 2023 Matthew Rogers
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
// File Name: Light
// Date File Created: 8/16/2023 3:34:50 PM
// Author: Matt
//
// ------------------------------------------------------------------------------

#pragma once


#include "../Common.h"

namespace lotus::graphics
{
L_TYPED_ID(light_id);

class light
{
public:
    enum type : u32
    {
        directional,
        point,
        spot,

        count
    };

    constexpr light() = default;
    constexpr explicit light(const light_id id, u64 light_set_key) : m_id(id), m_light_set_key(light_set_key) {}

    constexpr light_id get_id() const { return m_id; }
    constexpr u64      light_set_key() const { return m_light_set_key; }
    constexpr bool     is_valid() const { return id::is_valid(m_id); }

    void is_enabled(bool enabled) const;
    void intensity(f32 intensity) const;
    void color(vec3 color) const;

    bool is_enabled() const;
    f32 intensity() const;
    vec3 color() const;


    type        light_type() const;
    id::id_type entity_id() const;

private:
    light_id m_id{ id::invalid_id };
    u64      m_light_set_key{};
};
} // namespace lotus::graphics