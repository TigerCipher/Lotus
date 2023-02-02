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
#include "Lotus/Util/IOStream.h"


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

u64 get_vertex_element_size(elements::elements_type::type elements_type)
{
    using namespace elements;
    // clang-format off
    switch (elements_type)
    {
    case elements_type::static_color                    : return sizeof(static_color                 );
    case elements_type::static_normal                   : return sizeof(static_normal                );
    case elements_type::static_normal_texture           : return sizeof(static_normal_texture        );
    case elements_type::skeletal                        : return sizeof(skeletal                     );
    case elements_type::skeletal_color                  : return sizeof(skeletal_color               );
    case elements_type::skeletal_normal                 : return sizeof(skeletal_normal              );
    case elements_type::skeletal_normal_color           : return sizeof(skeletal_normal_color        );
    case elements_type::skeletal_normal_texture         : return sizeof(skeletal_normal_texture      );
    case elements_type::skeletal_normal_texture_color   : return sizeof(skeletal_normal_texture_color);
    }
    // clang-format on

    return 0;
}

void pack_vertices(mesh& m)
{
    const u32 num_verts = (u32) m.vertices.size();
    LASSERT(num_verts);

    m.position_buffer.resize(sizeof(vec3) * num_verts);
    vec3* const position_buffer = (vec3* const) m.position_buffer.data();

    for (u32 i = 0; i < num_verts; ++i)
    {
        position_buffer[i] = m.vertices[i].position;
    }

    struct u16v2
    {
        u16 x, y;
    };
    struct u8v3
    {
        u8 x, y, z;
    };

    utl::vector<u8>    t_signs(num_verts);
    utl::vector<u16v2> normals(num_verts);
    utl::vector<u16v2> tangents(num_verts);
    utl::vector<u8v3>  joint_weights(num_verts);

    if (m.elements_type & elements::elements_type::static_normal)
    {
        // Normals only
        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex& v  = m.vertices[i];
            t_signs[i] = (u8) ((v.normal.z > 0.0f) << 1);
            normals[i] = { (u16) math::pack_float<16>(v.normal.x, -1.0f, 1.0f),
                           (u16) math::pack_float<16>(v.normal.y, -1.0f, 1.0f) };
        }

        if (m.elements_type & elements::elements_type::static_normal_texture)
        {
            // Full T space
            for (u32 i = 0; i < num_verts; ++i)
            {
                vertex& v = m.vertices[i];
                t_signs[i] |= (u8) ((v.tangent.w > 0.0f) && (v.tangent.z > 0.0f));
                tangents[i] = { (u16) math::pack_float<16>(v.tangent.x, -1.0f, 1.0f),
                                (u16) math::pack_float<16>(v.tangent.y, -1.0f, 1.0f) };
            }
        }
    }

    if (m.elements_type & elements::elements_type::skeletal)
    {
        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex& v = m.vertices[i];
            // Pack joint weights -> [0.0, 1.0] to [0..255]
            joint_weights[i] = {
                (u8) math::pack_unit_float<8>(v.joint_weights.x),
                (u8) math::pack_unit_float<8>(v.joint_weights.y),
                (u8) math::pack_unit_float<8>(v.joint_weights.z),
            };
        }
    }

    m.element_buffer.resize(get_vertex_element_size(m.elements_type) * num_verts);
    using namespace elements;

    switch (m.elements_type)
    {
    case elements_type::static_color:
    {
        auto* const element_buffer{ (static_color* const) m.element_buffer.data() };

        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex& v         = m.vertices[i];
            element_buffer[i] = { { v.red, v.green, v.blue }, { /*pad*/ } };
        }
    }
    break;
    case elements_type::static_normal:
    {
        auto* const element_buffer{ (static_normal* const) m.element_buffer.data() };

        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex& v         = m.vertices[i];
            element_buffer[i] = { { v.red, v.green, v.blue }, t_signs[i], { normals[i].x, normals[i].y } };
        }
    }
    break;
    case elements_type::static_normal_texture:
    {
        auto* const element_buffer{ (static_normal_texture* const) m.element_buffer.data() };

        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex& v         = m.vertices[i];
            element_buffer[i] = { { v.red, v.green, v.blue },
                                  t_signs[i],
                                  { normals[i].x, normals[i].y },
                                  { tangents[i].x, tangents[i].y },
                                  v.uv };
        }
    }
    break;
    case elements_type::skeletal:
    {
        auto* const element_buffer{ (skeletal* const) m.element_buffer.data() };

        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex&   v = m.vertices[i];
            const u16 indices[4]{
                (u16) v.joint_indices.x,
                (u16) v.joint_indices.y,
                (u16) v.joint_indices.z,
                (u16) v.joint_indices.w,
            };
            element_buffer[i] = { { joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
                                  { /*pad*/ },
                                  { indices[0], indices[1], indices[2], indices[3] } };
        }
    }
    break;
    case elements_type::skeletal_color:
    {
        auto* const element_buffer{ (skeletal_color* const) m.element_buffer.data() };

        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex&   v = m.vertices[i];
            const u16 indices[4]{
                (u16) v.joint_indices.x,
                (u16) v.joint_indices.y,
                (u16) v.joint_indices.z,
                (u16) v.joint_indices.w,
            };
            element_buffer[i] = { { joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
                                  { /*pad*/ },
                                  { indices[0], indices[1], indices[2], indices[3] },
                                  { v.red, v.green, v.blue },
                                  { /*pad2*/ } };
        }
    }
    break;
    case elements_type::skeletal_normal:
    {
        auto* const element_buffer{ (skeletal_normal* const) m.element_buffer.data() };

        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex&   v = m.vertices[i];
            const u16 indices[4]{
                (u16) v.joint_indices.x,
                (u16) v.joint_indices.y,
                (u16) v.joint_indices.z,
                (u16) v.joint_indices.w,
            };
            element_buffer[i] = { { joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
                                  t_signs[i],
                                  { indices[0], indices[1], indices[2], indices[3] },
                                  { normals[i].x, normals[i].y } };
        }
    }
    break;
    case elements_type::skeletal_normal_color:
    {
        auto* const element_buffer{ (skeletal_normal_color* const) m.element_buffer.data() };

        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex&   v = m.vertices[i];
            const u16 indices[4]{
                (u16) v.joint_indices.x,
                (u16) v.joint_indices.y,
                (u16) v.joint_indices.z,
                (u16) v.joint_indices.w,
            };
            element_buffer[i] = { { joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
                                  t_signs[i],
                                  { indices[0], indices[1], indices[2], indices[3] },
                                  { normals[i].x, normals[i].y },
                                  { v.red, v.green, v.blue },
                                  { /*pad*/ } };
        }
    }
    break;
    case elements_type::skeletal_normal_texture:
    {
        auto* const element_buffer{ (skeletal_normal_texture* const) m.element_buffer.data() };

        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex&   v = m.vertices[i];
            const u16 indices[4]{
                (u16) v.joint_indices.x,
                (u16) v.joint_indices.y,
                (u16) v.joint_indices.z,
                (u16) v.joint_indices.w,
            };
            element_buffer[i] = { { joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
                                  t_signs[i],
                                  { indices[0], indices[1], indices[2], indices[3] },
                                  { normals[i].x, normals[i].y },
                                  { tangents[i].x, tangents[i].y },
                                  v.uv };
        }
    }
    break;
    case elements_type::skeletal_normal_texture_color:
    {
        auto* const element_buffer{ (skeletal_normal_texture_color* const) m.element_buffer.data() };

        for (u32 i = 0; i < num_verts; ++i)
        {
            vertex&   v = m.vertices[i];
            const u16 indices[4]{
                (u16) v.joint_indices.x,
                (u16) v.joint_indices.y,
                (u16) v.joint_indices.z,
                (u16) v.joint_indices.w,
            };
            element_buffer[i] = { { joint_weights[i].x, joint_weights[i].y, joint_weights[i].z },
                                  t_signs[i],
                                  { indices[0], indices[1], indices[2], indices[3] },
                                  { normals[i].x, normals[i].y },
                                  { tangents[i].x, tangents[i].y },
                                  v.uv,
                                  { v.red, v.green, v.blue },
                                  { /*pad*/ } };
        }
    }
    break;
    }
}

void determine_elements_type(mesh& m)
{
    using namespace elements;
    if (m.normals.size())
    {
        if (m.uv_sets.size() && m.uv_sets[0].size())
        {
            m.elements_type = elements_type::static_normal_texture;
        } else
        {
            m.elements_type = elements_type::static_normal;
        }
    } else if (m.colors.size())
    {
        m.elements_type = elements_type::static_color;
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

    determine_elements_type(m);
    pack_vertices(m);
}

u64 get_mesh_size(const mesh& m)
{
    const u64 num_verts            = m.vertices.size();
    const u64 position_buffer_size = m.position_buffer.size();
    LASSERT(position_buffer_size == sizeof(vec3) * num_verts);
    const u64 element_buffer_size = m.element_buffer.size();
    LASSERT(element_buffer_size == get_vertex_element_size(m.elements_type) * num_verts);
    const u64     index_size        = num_verts < (1 << 16) ? sizeof(u16) : sizeof(u32);
    const u64     index_buffer_size = index_size * m.indices.size();
    constexpr u64 size32            = sizeof(u32);

    const u64 size = size32 + m.name.size() + // mesh name len
                     size32 +                 // mesh id
                     size32 +                 // vertex element size (no positions)
                     size32 +                 // element type
                     size32 +                 // num vertices
                     size32 +                 // index size
                     size32 +                 // num indices
                     sizeof(f32) +            // lod threshold
                     position_buffer_size +   // space for positions
                     element_buffer_size +    // space for elements
                     index_buffer_size;       // space for indices


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

void pack_mesh_data(const mesh& m, utl::blob_stream_writer& blob)
{
    // mesh name
    blob.write((u32) m.name.size());
    blob.write(m.name.c_str(), m.name.size());
    // lod id
    blob.write(m.lod_id);
    // vertex element size
    const u32 elements_size = (u32) get_vertex_element_size(m.elements_type);
    blob.write(elements_size);
    // elements type enumeration
    blob.write((u32) m.elements_type);
    // number of vertices
    const u32 num_vertices = (u32) m.vertices.size();
    blob.write(num_vertices);
    // index size (16 bit or 32 bit)
    const u32 index_size = (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32);
    blob.write(index_size);
    // number of indices
    const u32 num_indices{ (u32) m.indices.size() };
    blob.write(num_indices);
    // LOD threshold
    blob.write(m.lod_threshold);
    // position buffer
    LASSERT(m.position_buffer.size() == sizeof(vec3) * num_vertices);
    blob.write(m.position_buffer.data(), m.position_buffer.size());
    // element buffer
    LASSERT(m.element_buffer.size() == elements_size * num_vertices);
    blob.write(m.element_buffer.data(), m.element_buffer.size());
    // index data
    const u32        index_buffer_size = index_size * num_indices;
    auto             data              = (const byte*) m.indices.data();
    utl::vector<u16> indices;


    if (index_size == sizeof(u16))
    {
        indices.resize(num_indices);
        for (u32 i = 0; i < num_indices; ++i)
        {
            indices[i] = (u16) m.indices[i];
        }
        data = (const byte*) indices.data();
    }

    blob.write(data, index_buffer_size);
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

    utl::blob_stream_writer blob(data.buffer, data.buffer_size);

    // scene name
    blob.write((u32) scene.name.size());
    blob.write(scene.name.c_str(), scene.name.size());
    // number of LODs
    blob.write((u32) scene.lod_groups.size());

    for (const auto& [name, meshes] : scene.lod_groups)
    {
        // LOD name
        blob.write((u32) name.size());
        blob.write(name.c_str(), name.size());
        // number of meshes in this LOD
        blob.write((u32) meshes.size());

        for (auto& m : meshes)
        {
            pack_mesh_data(m, blob);
        }
    }

    LASSERT(scene_size == blob.offset());
}

} // namespace lotus::tools
