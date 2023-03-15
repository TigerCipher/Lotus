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

struct views_cache
{
    D3D12_GPU_VIRTUAL_ADDRESS* const position_buffers;
    D3D12_GPU_VIRTUAL_ADDRESS* const element_buffers;
    D3D12_INDEX_BUFFER_VIEW* const   index_buffer_views;
    D3D_PRIMITIVE_TOPOLOGY* const    primitive_topologies;
    u32* const                       elements_types;
};

id::id_type add(const u8*& data);
void        remove(id::id_type id);
void        get_views(const id::id_type* const gpu_ids, u32 id_count, const views_cache& cache);
} // namespace submesh


namespace texture
{
id::id_type add(const u8* const);
void        remove(id::id_type);
void        get_descriptor_indices(const id::id_type* const texture_ids, u32 id_count, u32* const indices);
} // namespace texture

namespace material
{

struct materials_cache
{
    ID3D12RootSignature** const root_signatures;
    material_type::type* const  material_types;
};

id::id_type add(material_init_info info);
void        remove(id::id_type id);
void        get_materials(const id::id_type* const material_ids, u32 material_count, const materials_cache& cache);
} // namespace material


namespace render_item
{

struct items_cache
{
    id::id_type* const          entity_ids;
    id::id_type* const          submesh_gpu_ids;
    id::id_type* const          material_ids;
    ID3D12PipelineState** const psos;
    ID3D12PipelineState** const depth_psos;
};

id::id_type add(id::id_type entity_id, id::id_type geometry_content_id, u32 material_count,
                const id::id_type* const material_ids);
void        remove(id::id_type id);
void        get_d3d12_render_item_ids(const frame_info& info, utl::vector<id::id_type>& d3d12_render_item_ids);
void        get_items(const id::id_type* const d3d12_render_item_ids, u32 id_count, const items_cache& cache);

} // namespace render_item

} // namespace lotus::graphics::d3d12::content