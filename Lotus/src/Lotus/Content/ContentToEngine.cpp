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
//  File Name: ContentToEngine.cpp
//  Date File Created: 02/21/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#include "ContentToEngine.h"

#include "Lotus/Util/IOStream.h"
#include "Lotus/Graphics/Renderer.h"

namespace lotus::content
{

namespace
{

class geometry_hiearchy_stream
{
public:
    struct lod_offset
    {
        u16 offset;
        u16 count;
    };

    DISABLE_COPY_AND_MOVE(geometry_hiearchy_stream);

    geometry_hiearchy_stream(u8* const buffer, u32 lods = invalid_id_u32) : m_buffer(buffer)
    {
        LASSERT(buffer && lods);
        if (lods != invalid_id_u32)
        {
            *(u32*) buffer = lods;
        }

        m_lod_count   = *(u32*) buffer;
        m_thresholds  = (f32*) &buffer[sizeof(u32)];
        m_lod_offsets = (lod_offset*) &m_thresholds[m_lod_count];
        m_gpu_ids     = (id::id_type*) &m_lod_offsets[m_lod_count];
    }

    void gpu_ids(u32 lod, id::id_type*& ids, u32& id_count)
    {
        LASSERT(lod < m_lod_count);
        ids      = &m_gpu_ids[m_lod_offsets[lod].offset];
        id_count = m_lod_offsets[lod].count;
    }

    u32 lod_from_threshold(f32 threshold)
    {
        LASSERT(threshold > 0);

        for (u32 i = m_lod_count - 1; i > 0; --i)
        {
            if (m_thresholds[i] <= threshold)
                return i;
        }

        LASSERT(false);
        return 0;
    }

    [[nodiscard]] constexpr f32*         thresholds() const { return m_thresholds; }
    [[nodiscard]] constexpr lod_offset*  lod_offsets() const { return m_lod_offsets; }
    [[nodiscard]] constexpr id::id_type* gpu_ids() const { return m_gpu_ids; }
    [[nodiscard]] constexpr u32          lod_count() const { return m_lod_count; }

private:
    u8* const    m_buffer;
    f32*         m_thresholds;
    lod_offset*  m_lod_offsets;
    id::id_type* m_gpu_ids;
    u32          m_lod_count;
};

utl::free_list<u8*> geometry_hierarchies;
std::mutex          geometry_mutex;

u32 get_geometry_hierarchy_size(const void* const data)
{
    LASSERT(data);
    utl::blob_stream_reader blob((const u8*) data);

    const u32 lod_count = blob.read<u32>();
    LASSERT(lod_count);
    u32 size = sizeof(u32) + sizeof(f32) + sizeof(geometry_hiearchy_stream::lod_offset) * lod_count;

    for (u32 i = 0; i < lod_count; ++i)
    {
        blob.skip(sizeof(f32));                         // skip threshold
        size += sizeof(id::id_type) * blob.read<u32>(); // gpuid size .. id size * submesh count
        blob.skip(blob.read<u32>());
    }

    return size;
}

id::id_type create_mesh_hierarchy(const void* const data)
{
    LASSERT(data);
    const u32  size             = get_geometry_hierarchy_size(data);
    auto const hierarchy_buffer = (u8* const) malloc(size);

    utl::blob_stream_reader blob((const u8*) data);
    const u32               lod_count = blob.read<u32>();
    LASSERT(lod_count);
    geometry_hiearchy_stream stream(hierarchy_buffer, lod_count);

    u16                submesh_index = 0;
    id::id_type* const gpu_ids       = stream.gpu_ids();

    for (u32 lod_idx = 0; lod_idx < lod_count; ++lod_idx)
    {
        stream.thresholds()[lod_idx] = blob.read<f32>();
        const u32 id_count           = blob.read<u32>();
        LASSERT(id_count < (1 << 16));
        stream.lod_offsets()[lod_idx] = { submesh_index, (u16) id_count };
        blob.skip(sizeof(u32)); // Skip size_of_submeshes

        for (u32 id_idx = 0; id_idx < id_count; ++id_idx)
        {
            const u8* at             = blob.position();
            gpu_ids[submesh_index++] = graphics::add_submesh(at);
            blob.skip((u32) (at - blob.position()));
            LASSERT(submesh_index < (1 << 16));
        }
    }

    LASSERT([&] {
        f32 prev_threshold = stream.thresholds()[0];
        for (u32 i = 0; i < lod_count; ++i)
        {
            if (stream.thresholds()[i] <= prev_threshold)
                return false;
            prev_threshold = stream.thresholds()[i];
        }
        return true;
    }());

    std::lock_guard lock(geometry_mutex);

    return geometry_hierarchies.add(hierarchy_buffer);
}

// Data should contain the following format:
// struct {
//      u32 lod_count,
//      struct {
//          f32 lod_threshold,
//          u32 submesh_count,
//          u32 size_of_submeshes,
//          struct {
//              u32 element_size, u32 vertex_count,
//              u32 index_count, u32 elements_type, u32 primitive_topology,
//              u8 positions[sizeof(f32) * 3 * vertex_count],
//              u8 elements[sizeof(element_size) * vertex_count],
//              u8 indices[index_size * index_count]
//          } submeshes[submesh_count]
//      } mesh_lods[lod_count]
// } geometry
//
// Output will be in format:
//
// struct {
//      u32 lod_count,
//      f32 thresholds[lod_count],
//      struct {
//          u16 offset,
//          u16 count
//      } lod_offsets[lod_count]
//      id::id_type gpu_ids[total_number_submeshes]
// } geometry_hierarchy
id::id_type create_geometry_resource(const void* const data)
{
    return create_mesh_hierarchy(data);
}

void destroy_geometry_resource(id::id_type id)
{
    std::lock_guard lock(geometry_mutex);

    u8* const pointer = geometry_hierarchies[id];

    geometry_hiearchy_stream stream(pointer);
    const u32 lod_count = stream.lod_count();

    u32 id_idx = 0;
    for (u32 lod = 0; lod < lod_count; ++lod)
    {
        for (u32 i = 0; i < stream.lod_offsets()[lod].count; ++i)
        {
            graphics::remove_submesh(stream.gpu_ids()[id_idx++]);
        }
    }

    free(pointer);

    geometry_hierarchies.remove(id);
}

} // anonymous namespace


id::id_type create_resource(const void* const data, asset_type::type type)
{
    LASSERT(data);
    id::id_type id = invalid_id_u32;

    switch (type)
    {
    case asset_type::animation: break;
    case asset_type::audio: break;
    case asset_type::material: break;
    case asset_type::mesh: id = create_geometry_resource(data); break;
    case asset_type::skeleton: break;
    case asset_type::texture: break;
    }

    LASSERT(id::is_valid(id));
    return id;
}


void destroy_resource(id::id_type id, asset_type::type type)
{
    switch (type)
    {
    case asset_type::animation: break;
    case asset_type::audio: break;
    case asset_type::material: break;
    case asset_type::mesh: destroy_geometry_resource(id); break;
    case asset_type::skeleton: break;
    case asset_type::texture: break;
    default: LASSERT(false); break;
    }
}

} // namespace lotus::content
