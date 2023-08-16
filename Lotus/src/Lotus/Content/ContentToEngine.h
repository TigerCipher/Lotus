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

typedef struct compiled_shader
{
    static constexpr u32 hash_length = 16;

    [[nodiscard]] constexpr u64       byte_code_size() const { return m_byte_code_size; }
    [[nodiscard]] constexpr const u8* hash() const { return &m_hash[0]; }
    [[nodiscard]] constexpr const u8* byte_code() const { return &m_byte_code; }
    [[nodiscard]] constexpr const u64 buffer_size() const { return sizeof(m_byte_code_size) + hash_length + m_byte_code_size; }
    constexpr static u64              buffer_size(u64 size) { return sizeof(m_byte_code_size) + hash_length + size; }

private:
    u64 m_byte_code_size{};
    u8  m_hash[hash_length]{};
    u8  m_byte_code{};
} const* compiled_shader_ptr;

struct lod_offset
{
    u16 offset;
    u16 count;
};

id::id_type create_resource(const void* const data, asset_type::type type);
void        destroy_resource(id::id_type id, asset_type::type type);

id::id_type         add_shader_group(const u8** shaders, u32 num_shaders, const u32* const keys);
void                remove_shader_group(id::id_type id);
compiled_shader_ptr get_shader(id::id_type id, u32 shader_key);

void get_submesh_gpu_ids(id::id_type geometry_content_id, u32 id_count, id::id_type* const gpu_ids);
void get_lod_offsets(const id::id_type* const geometry_ids, const f32* const thresholds, u32 id_count,
                     utl::vector<lod_offset>& offsets);

} // namespace lotus::content