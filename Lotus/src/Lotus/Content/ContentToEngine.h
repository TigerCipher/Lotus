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
//  File Name: ContentToEngine.h
//  Date File Created: 02/19/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#pragma once
#include "Common.h"

namespace lotus::content
{

struct asset_type
{
    enum type : u32
    {
        unknown = 0,
        animation,
        audio,
        material,
        mesh,
        skeleton,
        texture,

        count
    };
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

id::id_type create_resource(const void* const data, asset_type::type type);

void destroy_resource(id::id_type id, asset_type::type type);

} // namespace lotus::content