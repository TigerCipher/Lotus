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
//  File Name: D3D12Content.h
//  Date File Created: 02/19/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#pragma once

#include "D3D12Common.h"

namespace lotus::graphics::d3d12::content
{

bool initialize();
void shutdown();

namespace submesh
{
id::id_type add(const u8*& data);
void        remove(id::id_type id);
} // namespace submesh


namespace texture
{
id::id_type add(const u8* const);
void        remove(id::id_type);
void        get_descriptor_indices(const id::id_type* const texture_ids, u32 id_count, u32* const indices);
} // namespace texture

namespace material
{
id::id_type add(material_init_info info);
void        remove(id::id_type id);
} // namespace material

} // namespace lotus::graphics::d3d12::content