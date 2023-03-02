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
// File Name: TransformComponent.h
// Date File Created: 8/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once
#include "../Components/Components.h"

namespace lotus::transform
{
L_TYPED_ID(transform_id)


class component final
{
public:
    constexpr explicit component(const transform_id id) : m_id(id) {}
    constexpr component() : m_id(id::invalid_id) {}

    constexpr transform_id get_id() const { return m_id; }

    constexpr bool is_valid() const { return id::is_valid(m_id); }

    vec3 position() const;
    vec4 rotation() const;
    vec3 scale() const;
    vec3 orientation() const;

private:
    transform_id m_id;
};
} // namespace lotus::transform
