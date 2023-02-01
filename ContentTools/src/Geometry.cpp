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
// File Name: Geometry.cpp
// Date File Created: 9/10/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "Geometry.h"


using namespace DirectX; // need this to use the overloaded operators

namespace lotus::tools
{

namespace
{

void recalculate_normals(mesh& m)
{
    const u32 num_indices = (u32) m.raw_indices.size();
    m.normals.resize(num_indices);

    for (u32 i = 0; i < num_indices; ++i)
    {
        const u32 i0 = m.raw_indices[i];
        const u32 i1 = m.raw_indices[++i];
        const u32 i2 = m.raw_indices[++i];

        const vec v0 = math::load_float3(&m.positions[i0]);
        const vec v1 = math::load_float3(&m.positions[i1]);
        const vec v2 = math::load_float3(&m.positions[i2]);


        const vec e0 = v1 - v0;
        const vec e1 = v2 - v0;

        vec n = math::normalize_vec3(math::cross_vec3(e0, e1));

        math::store_float3(&m.normals[i], n);
        m.normals[i - 1] = m.normals[i];
        m.normals[i - 2] = m.normals[i];
    }
}

void process_normals(mesh& m, f32 angle)
{
    const f32  cos_alpha = math::scalar_cos(pi - angle * pi / 180.0f);
    const bool hard      = math::scalar_near_equal(angle, 180.0f);
    const bool soft      = math::scalar_near_equal(angle, 0.0f);

    const u32 num_indices  = (u32) m.raw_indices.size();
    const u32 num_vertices = (u32) m.positions.size();
    LASSERT(num_indices && num_vertices);
    m.indices.resize(num_indices);

    utl::vector<utl::vector<u32>> index_ref(num_vertices);
    for (u32 i = 0; i < num_indices; ++i)
    {
        index_ref[m.raw_indices[i]].emplace_back(i);
    }

    for (u32 i = 0; i < num_vertices; ++i)
    {
        auto& refs     = index_ref[i];
        u32   num_refs = (u32) refs.size();
        for (u32 j = 0; j < num_refs; ++j)
        {
            m.indices[refs[j]] = (u32) m.vertices.size();

            vertex& v  = m.vertices.emplace_back();
            v.position = m.positions[m.raw_indices[refs[j]]];

            vec n1 = math::load_float3(&m.normals[refs[j]]);
            if (!hard)
            {
                for (u32 k = j + 1; k < num_refs; ++k)
                {
                    // cos(angle) between normals
                    f32       cos_theta = 0.0f;
                    const vec n2        = math::load_float3(&m.normals[refs[k]]);
                    if (!soft)
                    {
                        // cos(angle) = dot(n1, n2) / (|n1| * |n2|) ---- we assume n2 is unit length, so it is taken out of the formula
                        math::store_float(&cos_theta, math::dot_vec3(n1, n2) * math::reciprocal_length_vec3(n1));
                    }

                    if (soft || cos_theta >= cos_alpha)
                    {
                        n1 += n2;
                        m.indices[refs[k]] = m.indices[refs[j]];
                        refs.erase(refs.begin() + k);
                        --num_refs;
                        --k;
                    }
                }
            }
            math::store_float3(&v.normal, math::normalize_vec3(n1));
        }
    }
}

void process_uvs(mesh& m)
{
    utl::vector<vertex> old_verts;
    old_verts.swap(m.vertices);
    utl::vector<u32> old_indices(m.indices.size());
    old_indices.swap(m.indices);

    const u32 num_verts   = (u32) old_verts.size();
    const u32 num_indices = (u32) old_indices.size();
    LASSERT(num_verts && num_indices);

    utl::vector<utl::vector<u32>> index_ref(num_verts);
    for (u32 i = 0; i < num_indices; ++i)
    {
        index_ref[old_indices[i]].emplace_back(i);
    }

    for (u32 i = 0; i < num_verts; ++i)
    {
        auto& refs     = index_ref[i];
        u32   num_refs = (u32) refs.size();

        for (u32 j = 0; j < num_refs; ++j)
        {
            m.indices[refs[j]] = (u32) m.vertices.size();
            vertex& v          = old_verts[old_indices[refs[j]]];
            v.uv               = m.uv_sets[0][refs[j]];
            m.vertices.emplace_back(v);

            for (u32 k = j + 1; k < num_refs; ++k)
            {
                const vec2& uv1 = m.uv_sets[0][refs[k]];
                if (math::scalar_near_equal(v.uv.x, uv1.x) && math::scalar_near_equal(v.uv.y, uv1.y))
                {
                    m.indices[refs[k]] = m.indices[refs[j]];
                    refs.erase(refs.begin() + k);
                    --num_refs;
                    --k;
                }
            }
        }
    }
}

void pack_vertices(mesh& m)
{
    const u32 num_verts = (u32) m.vertices.size();
    LASSERT(num_verts);
    m.packed_vertices_static.reserve(num_verts);

    for (u32 i = 0; i < num_verts; ++i)
    {
        const auto& [tangent, position, normal, uv] = m.vertices[i];
        const u8  signs                             = (u8)((normal.z > 0.0f) << 1);
        const u16 n_x                               = (u16) math::pack_float<16>(normal.x, -1.0f, 1.0f);
        const u16 n_y                               = (u16) math::pack_float<16>(normal.y, -1.0f, 1.0f);

        // #TODO: Pack tangents

        m.packed_vertices_static.emplace_back(
            packed_vertex::vertex_static{ position, { 0, 0, 0 }, signs, { n_x, n_y }, {}, uv });
    }
}

void process_vertices(mesh& m, const geometry_import_settings& settings)
{
    LASSERT(m.raw_indices.size() % 3 == 0);
    if (settings.calculate_normals || m.normals.empty())
    {
        recalculate_normals(m);
    }

    process_normals(m, settings.smoothing_angle);

    if (!m.uv_sets.empty())
    {
        process_uvs(m);
    }

    pack_vertices(m);
}

u64 get_mesh_size(const mesh& m)
{
    const u64     num_verts         = m.vertices.size();
    const u64     vert_buffer_size  = sizeof(packed_vertex::vertex_static) * num_verts;
    const u64     index_size        = num_verts < 1 << 16 ? sizeof(u16) : sizeof(u32);
    const u64     index_buffer_size = index_size * m.indices.size();
    constexpr u64 size32            = sizeof(u32);

    const u64 size = size32 + m.name.size() + // mesh name len
                     size32 +                 // mesh id
                     size32 +                 // vertex size
                     size32 +                 // num vertices
                     size32 +                 // index size
                     size32 +                 // num indices
                     sizeof(f32) +            // lod threshold
                     vert_buffer_size + index_buffer_size;


    return size;
}

u64 get_scene_size(const scene& scene)
{
    constexpr u64 size32 = sizeof(u32);

    u64 size = size32 + scene.name.size() + size32;

    for (auto& lod : scene.lod_groups)
    {
        u64 lod_size = size32 + lod.name.size() + size32;

        for (auto& m : lod.meshes)
        {
            lod_size += get_mesh_size(m);
        }
        size += lod_size;
    }

    return size;
}

void pack_mesh_data(const mesh& m, u8* const buffer, u64& at)
{
    constexpr u64 size32 = sizeof(u32);
    u32           s      = 0;

    // Mesh name
    s = (u32) m.name.size();
    memcpy(&buffer[at], &s, size32);
    at += size32;

    memcpy(&buffer[at], m.name.c_str(), s);
    at += s;

    // lod id
    s = m.lod_id;
    memcpy(&buffer[at], &s, size32);
    at += size32;

    // vertex size
    constexpr u32 vertex_size = sizeof(packed_vertex::vertex_static);
    s                         = vertex_size;
    memcpy(&buffer[at], &s, size32);
    at += size32;

    // num verts
    const u32 num_verts = (u32) m.vertices.size();
    s                   = num_verts;
    memcpy(&buffer[at], &s, size32);
    at += size32;

    // Index Size
    const u32 index_size = num_verts < 1 << 16 ? sizeof(u16) : sizeof(u32);
    s                    = index_size;
    memcpy(&buffer[at], &s, size32);
    at += size32;

    // Num indices
    const u32 num_indices = (u32) m.indices.size();
    s                     = num_indices;
    memcpy(&buffer[at], &s, size32);
    at += size32;

    // lod threshold
    memcpy(&buffer[at], &m.lod_threshold, sizeof(f32));
    at += sizeof(f32);

    // Vertex data
    s = vertex_size * num_verts;
    memcpy(&buffer[at], m.packed_vertices_static.data(), s);
    at += s;

    // Index data
    s         = index_size * num_indices;
    auto data = (void*) m.indices.data();

    utl::vector<u16> indices;
    if (index_size == sizeof(u16))
    {
        indices.resize(num_indices);
        for (u32 i = 0; i < num_indices; ++i)
        {
            indices[i] = (u16) m.indices[i];
        }
        data = (void*) indices.data();
    }
    memcpy(&buffer[at], data, s);
    at += s;
}

bool split_meshes_by_material(u32 mtl_idx, const mesh& m, mesh& submesh)
{
    submesh.name          = m.name;
    submesh.lod_threshold = m.lod_threshold;
    submesh.lod_id        = m.lod_id;
    submesh.material_used.emplace_back(mtl_idx);
    submesh.uv_sets.resize(m.uv_sets.size());

    const u32   num_polys = (u32) m.raw_indices.size() / 3;
    utl::vector vertex_ref(m.positions.size(), invalid_id_u32);

    for (u32 i = 0; i < num_polys; ++i)
    {
        if (mtl_idx != m.material_indices[i])
            continue;

        const u32 index = i * 3;
        for (u32 j = index; j < index + 3; ++j)
        {
            const u32 v_idx{ m.raw_indices[j] };
            if (vertex_ref[v_idx] != invalid_id_u32)
            {
                submesh.raw_indices.emplace_back(vertex_ref[v_idx]);
            } else
            {
                submesh.raw_indices.emplace_back((u32) submesh.positions.size());
                vertex_ref[v_idx] = submesh.raw_indices.back();
                submesh.positions.emplace_back(m.positions[v_idx]);
            }

            if (m.normals.size())
            {
                submesh.normals.emplace_back(m.normals[j]);
            }

            if (m.tangents.size())
            {
                submesh.tangents.emplace_back(m.tangents[j]);
            }

            for (u32 k = 0; k < m.uv_sets.size(); ++k)
            {
                if (m.uv_sets[k].size())
                {
                    submesh.uv_sets[k].emplace_back(m.uv_sets[k][j]);
                }
            }
        }
    }

    assert(submesh.raw_indices.size() % 3 == 0);
    return !submesh.raw_indices.empty();
}

void split_meshes_by_material(scene& scene)
{
    for (auto& [name, meshes] : scene.lod_groups)
    {
        utl::vector<mesh> new_meshes;

        for (auto& m : meshes)
        {
            const u32 num_materials = (u32) m.material_used.size();
            if (num_materials > 1)
            {
                for (u32 i = 0; i < num_materials; ++i)
                {
                    if (mesh submesh{}; split_meshes_by_material(m.material_used[i], m, submesh))
                    {
                        new_meshes.emplace_back(submesh);
                    }
                }
            } else
            {
                new_meshes.emplace_back(m);
            }
        }

        new_meshes.swap(meshes);
    }
}

} // anonymous namespace

void process_scene(scene& scene, const geometry_import_settings& settings)
{
    split_meshes_by_material(scene);
    for (auto& [name, meshes] : scene.lod_groups)
    {
        for (auto& m : meshes)
        {
            process_vertices(m, settings);
        }
    }
}


void pack_data(const scene& scene, scene_data& data)
{
    constexpr u64 size32     = sizeof(u32);
    const u64     scene_size = get_scene_size(scene);
    data.buffer_size         = (u32) scene_size;
    data.buffer              = (u8*) CoTaskMemAlloc(scene_size);
    LASSERT(data.buffer);

    u8* const buffer = data.buffer;
    u64       at     = 0;
    u32       s      = 0;

    // scene name
    s = (u32) scene.name.size();
    memcpy(&buffer[at], &s, size32);
    at += size32;

    memcpy(&buffer[at], scene.name.c_str(), s);
    at += s;

    // Number of lods
    s = (u32) scene.lod_groups.size();
    memcpy(&buffer[at], &s, size32);
    at += size32;

    for (auto& lod : scene.lod_groups)
    {
        s = (u32) lod.name.size();
        memcpy(&buffer[at], &s, size32);
        at += size32;

        memcpy(&buffer[at], lod.name.c_str(), s);
        at += s;

        // Number of meshes in lod
        s = (u32) lod.meshes.size();
        memcpy(&buffer[at], &s, size32);
        at += size32;

        for (auto& m : lod.meshes)
        {
            pack_mesh_data(m, buffer, at);
        }
    }

    LASSERT(scene_size == at);
}

} // namespace lotus::tools
