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
// File Name: Transform.h
// Date File Created: 8/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once
#include "Components.h"


namespace lotus::transform
{

struct create_info
{
    f32 position[3]{};
    f32 rotation[4]{};
    f32 scale[3]{ 1.0f, 1.0f, 1.0f };
};

struct component_flags
{
    enum flags : u32
    {
        rotation    = 0x01,
        orientation = 0x02,
        position    = 0x04,
        scale       = 0x08,

        all = rotation | orientation | position | scale
    };
};

struct component_cache
{
    vec4         rotation;
    vec3         orientation;
    vec3         position;
    vec3         scale;
    transform_id id;
    u32          flags;
};

component create(const create_info& info, game_entity::entity entity);
void      remove(component comp);
void      get_transform_matrices(const game_entity::entity_id id, mat4& world, mat4& inverse_world);
void      get_updated_components_flags(const game_entity::entity_id* const ids, u32 count, u8* const flags);
void      update(const component_cache* const cache, u32 count);

} // namespace lotus::transform
