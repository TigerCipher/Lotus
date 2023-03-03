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
//  File Name: D3D12Content.cpp
//  Date File Created: 02/19/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#include "D3D12Content.h"

#include "D3D12Core.h"
#include "Util/IOStream.h"
#include "Content/ContentToEngine.h"


namespace lotus::graphics::d3d12::content
{

namespace
{

struct submesh_view
{
    D3D12_VERTEX_BUFFER_VIEW position_buffer_view{};
    D3D12_VERTEX_BUFFER_VIEW element_buffer_view{};
    D3D12_INDEX_BUFFER_VIEW  index_buffer_view{};
    D3D_PRIMITIVE_TOPOLOGY   primitive_topology{};
    u32                      element_type{};
};

utl::free_list<ID3D12Resource*> submesh_buffers{};
utl::free_list<submesh_view>   submesh_views{};

std::mutex submesh_mutex{};


D3D_PRIMITIVE_TOPOLOGY get_d3d_primitive_topology(const primitive_topology::type type)
{
    using namespace lotus::content;
    LASSERT(type < primitive_topology::count);
    switch (type)
    {
    case primitive_topology::point_list: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
    case primitive_topology::line_list: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    case primitive_topology::line_strip: return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
    case primitive_topology::triangle_list: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case primitive_topology::triangle_strip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
    default: return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    }
}

} // anonymous namespace

namespace submesh
{

/**
 * \brief
 * Data should contain the following format:
 *
 * u32 element_size, u32 vertex_count,
 *
 * u32 index_count, u32 elements_type, u32 primitive_topology,
 *
 * u8 positions[sizeof(f32) * 3 * vertex_count],
 *
 * u8 elements[sizeof(element_size) * vertex_count],
 *
 * u8 indices[index_size * index_count]
 *
 * NOTE:
 * - This will advance the data pointer
 *
 * - Position and element buffers must be padded as a multiple of 4 bytes
 *
 * - - Defined as D3D12_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE
 */
id::id_type add(const byte*& data)
{
    utl::blob_stream_reader blob{ data };

    const u32 element_size       = blob.read<u32>();
    const u32 vertex_count       = blob.read<u32>();
    const u32 index_count        = blob.read<u32>();
    const u32 elements_type      = blob.read<u32>();
    const u32 primitive_topology = blob.read<u32>();
    const u32 index_size         = vertex_count < (1 << 16) ? sizeof(u16) : sizeof(u32);

    const u32 pos_buffer_size   = sizeof(vec3) * vertex_count;
    const u32 elem_buffer_size  = element_size * vertex_count;
    const u32 index_buffer_size = index_size * index_count;

    constexpr u32 alignment                = D3D12_STANDARD_MAXIMUM_ELEMENT_ALIGNMENT_BYTE_MULTIPLE;
    const u32     aligned_pos_buffer_size  = (u32) math::align_size_up<alignment>(pos_buffer_size);
    const u32     aligned_elem_buffer_size = (u32) math::align_size_up<alignment>(elem_buffer_size);

    const u32 total_buffer_size = aligned_pos_buffer_size + aligned_elem_buffer_size + index_buffer_size;

    ID3D12Resource* res = d3dx::create_buffer(blob.position(), total_buffer_size);

    blob.skip(total_buffer_size);
    data = blob.position();

    submesh_view view{};
    view.position_buffer_view.BufferLocation = res->GetGPUVirtualAddress();
    view.position_buffer_view.SizeInBytes    = pos_buffer_size;
    view.position_buffer_view.StrideInBytes  = sizeof(vec3);

    if (element_size)
    {
        view.element_buffer_view.BufferLocation = res->GetGPUVirtualAddress() + aligned_pos_buffer_size;
        view.element_buffer_view.SizeInBytes    = elem_buffer_size;
        view.element_buffer_view.StrideInBytes  = element_size;
    }

    view.index_buffer_view.BufferLocation = res->GetGPUVirtualAddress() + aligned_pos_buffer_size + aligned_elem_buffer_size;
    view.index_buffer_view.Format         = index_size == sizeof(u16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
    view.index_buffer_view.SizeInBytes    = index_buffer_size;


    view.primitive_topology = get_d3d_primitive_topology((primitive_topology::type) primitive_topology);
    view.element_type       = elements_type;


    std::lock_guard lock(submesh_mutex);

    submesh_buffers.add(res);
    return submesh_views.add(view);
}

void remove(id::id_type id)
{
    std::lock_guard lock(submesh_mutex);
    
    submesh_views.remove(id);

    core::deferred_release(submesh_buffers[id]);
    submesh_buffers.remove(id);
}

} // namespace submesh

} // namespace lotus::graphics::d3d12::content
