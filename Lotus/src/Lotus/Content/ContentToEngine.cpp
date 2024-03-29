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
//  File Name: ContentToEngine.cpp
//  Date File Created: 02/21/2023
//  Author: Matt
//
//  ------------------------------------------------------------------------------

#include "ContentToEngine.h"

#include "Util/IOStream.h"
#include "Graphics/Renderer.h"

namespace lotus::content
{

namespace
{

class geometry_hiearchy_stream
{
public:
    DISABLE_COPY_AND_MOVE(geometry_hiearchy_stream);

    geometry_hiearchy_stream(u8* const buffer, u32 lods = invalid_id_u32) : m_buffer(buffer)
    {
        assert(buffer && lods);
        if (lods != invalid_id_u32)
        {
            *(u32*) buffer = lods;
        }

        m_lod_count   = *(u32*) buffer;
        m_thresholds  = (f32*) &buffer[sizeof(u32)];
        m_lod_offsets = (lod_offset*) &m_thresholds[m_lod_count];
        m_gpu_ids     = (id::id_type*) &m_lod_offsets[m_lod_count];
    }

    void gpu_ids(u32 lod, id::id_type*& ids, u32& id_count) const
    {
        assert(lod < m_lod_count);
        ids      = &m_gpu_ids[m_lod_offsets[lod].offset];
        id_count = m_lod_offsets[lod].count;
    }

    u32 lod_from_threshold(f32 threshold) const
    {
        assert(threshold > 0);
        if (m_lod_count == 1)
            return 0;

        for (u32 i = m_lod_count - 1; i > 0; --i)
        {
            if (m_thresholds[i] <= threshold)
                return i;
        }

        assert(false);
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

struct noexcept_map
{
    std::unordered_map<u32, scope<u8[]>> map;
    noexcept_map()                                   = default;
    noexcept_map(const noexcept_map&)                = default;
    noexcept_map(noexcept_map&&) noexcept            = default;
    noexcept_map& operator=(const noexcept_map&)     = default;
    noexcept_map& operator=(noexcept_map&&) noexcept = default;
};

// Indicates an element in geometry_hiarchies is a fake pointer and is actually a gpu_id
constexpr uintptr_t single_mesh_marker = (uintptr_t) 0x01;

utl::free_list<u8*> geometry_hierarchies;
std::mutex          geometry_mutex;




utl::free_list<noexcept_map> shader_groups;
std::mutex                   shader_mutex;

u32 get_geometry_hierarchy_size(const void* const data)
{
    assert(data);
    utl::blob_stream_reader blob((const u8*) data);

    const u32 lod_count = blob.read<u32>();
    assert(lod_count);
    u32 size = sizeof(u32) + sizeof(f32) + sizeof(lod_offset) * lod_count;

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
    assert(data);
    const u32  size             = get_geometry_hierarchy_size(data);
    const auto hierarchy_buffer = (u8* const) malloc(size);

    utl::blob_stream_reader blob((const u8*) data);
    const u32               lod_count = blob.read<u32>();
    assert(lod_count);
    const geometry_hiearchy_stream stream{ hierarchy_buffer, lod_count };

    u32                submesh_index = 0;
    id::id_type* const gpu_ids       = stream.gpu_ids();

    for (u32 lod_idx = 0; lod_idx < lod_count; ++lod_idx)
    {
        stream.thresholds()[lod_idx] = blob.read<f32>();
        const u32 id_count           = blob.read<u32>();
        assert(id_count < (1 << 16));
        stream.lod_offsets()[lod_idx] = { (u16) submesh_index, (u16) id_count };
        blob.skip(sizeof(u32)); // Skip size_of_submeshes

        for (u32 id_idx = 0; id_idx < id_count; ++id_idx)
        {
            const u8* at             = blob.position();
            gpu_ids[submesh_index++] = graphics::add_submesh(at);
            blob.skip((u32) (at - blob.position()));
            assert(submesh_index < (1 << 16));
        }
    }

    assert([&] {
        f32 prev_threshold = stream.thresholds()[0];
        for (u32 i = 1; i < lod_count; ++i)
        {
            if (stream.thresholds()[i] <= prev_threshold)
                return false;
            prev_threshold = stream.thresholds()[i];
        }
        return true;
    }());

    static_assert(alignof(void*) > 2, "The least significant bit is needed for the single_mesh_marker");

    std::lock_guard lock(geometry_mutex);

    return geometry_hierarchies.add(hierarchy_buffer);
}

// Determines if geometry has a single LOD and a single submesh
bool is_single_mesh(const void* const data)
{
    assert(data);
    utl::blob_stream_reader blob((const u8*) data);
    const u32               lod_count = blob.read<u32>();
    assert(lod_count);
    if (lod_count > 1)
        return false;

    // Skip threshold
    blob.skip(sizeof(f32));
    const u32 submesh_count = blob.read<u32>();
    assert(submesh_count);

    return submesh_count == 1;
}


id::id_type create_single_submesh(const void* const data)
{
    assert(data);
    utl::blob_stream_reader blob((const u8*) data);

    // Skip lod count, threshold, submesh count, and submesh size
    blob.skip(sizeof(u32) + sizeof(f32) + sizeof(u32) + sizeof(u32));

    const u8*         at     = blob.position();
    const id::id_type gpu_id = graphics::add_submesh(at);

    // Create a fake pointer
    static_assert(sizeof(uintptr_t) > sizeof(id::id_type));
    constexpr u8 shift_bits = (sizeof(uintptr_t) - sizeof(id::id_type)) << 3;
    u8* const    fake_ptr   = (u8* const) (((uintptr_t) gpu_id << shift_bits) | single_mesh_marker);

    std::lock_guard lock(geometry_mutex);
    return geometry_hierarchies.add(fake_ptr);
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
    assert(data);
    return is_single_mesh(data) ? create_single_submesh(data) : create_mesh_hierarchy(data);
}

constexpr id::id_type gpu_id_from_fake_pointer(u8* const pointer)
{
    assert((uintptr_t) pointer & single_mesh_marker);
    static_assert(sizeof(uintptr_t) > sizeof(id::id_type));
    constexpr u8 shift_bits = (sizeof(uintptr_t) - sizeof(id::id_type)) << 3;
    return ((uintptr_t) pointer >> shift_bits) & (uintptr_t) id::invalid_id;
}

void destroy_geometry_resource(id::id_type id)
{
    std::lock_guard lock(geometry_mutex);

    u8* const pointer = geometry_hierarchies[id];
    // If the pointer is fake
    if ((uintptr_t) pointer & single_mesh_marker)
    {
        graphics::remove_submesh(gpu_id_from_fake_pointer(pointer));
    } else
    {
        const geometry_hiearchy_stream stream{ pointer };
        const u32                      lod_count = stream.lod_count();

        u32 id_idx = 0;
        for (u32 lod = 0; lod < lod_count; ++lod)
        {
            for (u32 i = 0; i < stream.lod_offsets()[lod].count; ++i)
            {
                graphics::remove_submesh(stream.gpu_ids()[id_idx++]);
            }
        }

        free(pointer);
    }

    geometry_hierarchies.remove(id);
}

// Data format should contain the following:
// struct {
//      material_type::type type,
//      u32 texture_count,
//      id::id_type shader_ids[shader_type::count],
//      id::id_type* texture_ids
// } material_init_info
id::id_type create_material_resource(const void* const data)
{
    assert(data);
    return graphics::add_material(*(const graphics::material_init_info* const) data);
}

void destroy_material_resource(id::id_type id)
{
    graphics::remove_material(id);
}

} // anonymous namespace


id::id_type create_resource(const void* const data, asset_type::type type)
{
    assert(data);
    id::id_type id = invalid_id_u32;

    switch (type)
    {
    case asset_type::animation: break;
    case asset_type::audio: break;
    case asset_type::material: id = create_material_resource(data); break;
    case asset_type::mesh: id = create_geometry_resource(data); break;
    case asset_type::skeleton: break;
    case asset_type::texture: break;
    }

    assert(id::is_valid(id));
    return id;
}


void destroy_resource(id::id_type id, asset_type::type type)
{
    switch (type)
    {
    case asset_type::animation: break;
    case asset_type::audio: break;
    case asset_type::material: destroy_material_resource(id); break;
    case asset_type::mesh: destroy_geometry_resource(id); break;
    case asset_type::skeleton: break;
    case asset_type::texture: break;
    default: assert(false); break;
    }
}

id::id_type add_shader_group(const u8** shaders, u32 num_shaders, const u32* const keys)
{
    assert(shaders && num_shaders && keys);
    noexcept_map group;
    for (u32 i = 0; i < num_shaders; ++i)
    {
        assert(shaders[i]);
        const auto  shader_ptr{ (const compiled_shader_ptr) shaders[i] };
        const u64   size{ shader_ptr->buffer_size() };
        scope<u8[]> shader{ create_scope<u8[]>(size) };
        memcpy(shader.get(), shaders[i], size);
        group.map[keys[i]] = std::move(shader);
    }


    std::lock_guard lock(shader_mutex);
    return shader_groups.add(std::move(group));
}

void remove_shader_group(id::id_type id)
{
    std::lock_guard lock(shader_mutex);
    assert(id::is_valid(id));
    shader_groups[id].map.clear();
    shader_groups.remove(id);
}

compiled_shader_ptr get_shader(id::id_type id, u32 shader_key)
{
    std::lock_guard lock(shader_mutex);
    assert(id::is_valid(id));

    for (const auto& [key, value] : shader_groups[id].map)
    {
        if (key == shader_key)
        {
            return (const compiled_shader_ptr) value.get();
        }
    }

    assert(false);
    return nullptr;
}

void get_submesh_gpu_ids(id::id_type geometry_content_id, u32 id_count, id::id_type* const gpu_ids)
{
    std::lock_guard lock(geometry_mutex);

    u8* const ptr = geometry_hierarchies[geometry_content_id];
    if ((uintptr_t) ptr & single_mesh_marker)
    {
        assert(id_count == 1);
        *gpu_ids = gpu_id_from_fake_pointer(ptr);
    } else
    {
        const geometry_hiearchy_stream stream{ ptr };

        assert([&] {
            const u32        lod_count = stream.lod_count();
            const lod_offset lodoffset{ stream.lod_offsets()[lod_count - 1] };
            const u32        gpu_id_count = (u32) lodoffset.offset + (u32) lodoffset.count;
            return gpu_id_count == id_count;
        }());

        memcpy(gpu_ids, stream.gpu_ids(), id::size * id_count);
    }
}

void get_lod_offsets(const id::id_type* const geometry_ids, const f32* const thresholds, u32 id_count,
                     utl::vector<lod_offset>& offsets)
{
    assert(geometry_ids && thresholds && id_count);
    assert(offsets.empty());

    std::lock_guard lock(geometry_mutex);
    for (u32 i = 0; i < id_count; ++i)
    {
        u8* const ptr = geometry_hierarchies[geometry_ids[i]];
        if ((uintptr_t) ptr & single_mesh_marker)
        {
            //assert(id_count == 1);
            offsets.emplace_back(lod_offset{ 0, 1 });
        } else
        {
            geometry_hiearchy_stream stream{ ptr };
            const u32                lod = stream.lod_from_threshold(thresholds[i]);
            offsets.emplace_back(stream.lod_offsets()[lod]);
        }
    }
}

} // namespace lotus::content
