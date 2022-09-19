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
        const u32 numIndices = (u32) m.rawIndices.size();
        m.normals.reserve(numIndices);

        for (u32 i = 0; i < numIndices; ++i)
        {
            //
            const u32 i0 = m.rawIndices [ i ];
            const u32 i1 = m.rawIndices [ ++i ];
            const u32 i2 = m.rawIndices [ ++i ];

            vec v0 = math::load_float3(&m.positions [ i0 ]);
            vec v1 = math::load_float3(&m.positions [ i1 ]);
            vec v2 = math::load_float3(&m.positions [ i2 ]);


            vec e0 = v1 - v0;
            vec e1 = v2 - v0;

            vec n = math::normalize_vec3(math::cross_vec3(e0, e1));

            math::store_float3(&m.normals [ i ], n);
            m.normals [ i - 1 ] = m.normals [ i ];
            m.normals [ i - 2 ] = m.normals [ i ];
        }
    }

    void process_normals(mesh& m, f32 angle)
    {
        const f32  cosAlpha = math::scalar_cos(pi - angle * pi / 180.0f);
        const bool hard     = math::scalar_near_equal(angle, 180.0f);
        const bool soft     = math::scalar_near_equal(angle, 0.0f);

        const u32 numIndices  = (u32) m.rawIndices.size();
        const u32 numVertices = (u32) m.positions.size();
        LASSERT(numIndices && numVertices);
        m.indices.resize(numIndices);

        utl::vector<utl::vector<u32>> indexRef(numVertices);
        for (u32 i = 0; i < numIndices; ++i) { indexRef [ m.rawIndices [ i ] ].emplace_back(i); }

        for (u32 i = 0; i < numVertices; ++i)
        {
            auto& refs    = indexRef [ i ];
            u32   numRefs = (u32) indexRef.size();
            for (u32 j = 0; j < numRefs; ++j)
            {
                m.indices [ refs [ j ] ] = (u32) m.vertices.size();

                vertex& v  = m.vertices.emplace_back();
                v.position = m.positions [ m.rawIndices [ refs [ j ] ] ];

                vec n1 = math::load_float3(&m.normals [ refs [ j ] ]);
                if (!hard)
                {
                    for (u32 k = j + 1; k < numRefs; ++k)
                    {
                        // cos(angle) between normals
                        f32 cosTheta = 0.0f;
                        vec n2       = math::load_float3(&m.normals [ refs [ k ] ]);
                        if (!soft)
                        {
                            // cos(angle) = dot(n1, n2) / (|n1| * |n2|) ---- we assume n2 is unit length, so it is taken out of the formula
                            math::store_float(&cosTheta, math::dot_vec3(n1, n2) * math::reciprocal_length_vec3(n1));
                        }

                        if (soft || cosTheta >= cosAlpha)
                        {
                            n1 += n2;
                            m.indices [ refs [ k ] ] = m.indices [ refs [ j ] ];
                            refs.erase(refs.begin() + k);
                            --numRefs;
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
        utl::vector<vertex> oldVerts;
        oldVerts.swap(m.vertices);
        utl::vector<u32> oldIndices(m.indices.size());
        oldIndices.swap(m.indices);

        const u32 numVerts   = (u32) oldVerts.size();
        const u32 numIndices = (u32) oldIndices.size();
        LASSERT(numVerts && numIndices);

        utl::vector<utl::vector<u32>> indexRef(numVerts);
        for (u32 i = 0; i < numIndices; ++i) { indexRef [ m.rawIndices [ i ] ].emplace_back(i); }

        for (u32 i = 0; i < numIndices; ++i)
        {
            auto& refs    = indexRef [ i ];
            u32   numRefs = (u32) refs.size();

            for (u32 j = 0; j < numRefs; ++j)
            {
                m.indices [ refs [ j ] ] = (u32) m.vertices.size();
                vertex& v                = oldVerts [ oldIndices [ refs [ j ] ] ];
                v.uv                     = m.uvSets [ 0 ][ refs [ j ] ];
                m.vertices.emplace_back(v);

                for (u32 k = j + 1; k < numRefs; ++k)
                {
                    vec2& uv1 = m.uvSets [ 0 ][ refs [ k ] ];
                    if (math::scalar_near_equal(v.uv.x, uv1.x) && math::scalar_near_equal(v.uv.y, uv1.y))
                    {
                        m.indices [ refs [ k ] ] = m.indices [ refs [ j ] ];
                        refs.erase(refs.begin() + k);
                        --numRefs;
                        --k;
                    }
                }
            }
        }
    }

    void pack_vertices(mesh& m)
    {
        const u32 numVerts = (u32) m.vertices.size();
        LASSERT(numVerts);
        m.packedVerticesStatic.reserve(numVerts);

        for (u32 i = 0; i < numVerts; ++i)
        {
            const auto& [ tangent, position, normal, uv ] = m.vertices [ i ];
            const u8  signs                               = ((u8) normal.z > 0.0f) << 1;
            const u16 nX                                  = (u16) math::pack_float<16>(normal.x, -1.0f, 1.0f);
            const u16 nY                                  = (u16) math::pack_float<16>(normal.y, -1.0f, 1.0f);

            // #TODO: Pack tangents

            m.packedVerticesStatic.emplace_back(
                packed_vertex::vertex_static { position, { 0, 0, 0 }, signs, { nX, nY }, {}, uv });
        }
    }

    void process_vertices(mesh& m, const geometry_import_settings& settings)
    {
        LASSERT(m.rawIndices.size() % 3 == 0);
        if (settings.calculateNormals || m.normals.empty()) { recalculate_normals(m); }

        process_normals(m, settings.smoothingAngle);

        if (!m.uvSets.empty()) { process_uvs(m); }

        pack_vertices(m);
    }

    u64 get_mesh_size(const mesh& m)
    {
        const u64     numVerts = m.vertices.size();
        const u64     vertBufferSize = sizeof(packed_vertex::vertex_static) * numVerts;
        const u64     indexSize = numVerts < 1 << 16 ? sizeof(u16) : sizeof(u32);
        const u64     indexBufferSize = indexSize * m.indices.size();
        constexpr u64 size32 = sizeof(u32);

        const u64 size = size32 + m.name.size() + // mesh name len
            size32 + // mesh id
            size32 + // vertex size
            size32 + // num vertices
            size32 + // index size
            size32 + // num indices
            sizeof(f32) + // lod threshold
            vertBufferSize + indexBufferSize;


        return size;
    }

    u64 get_scene_size(const scene& scene)
    {
        constexpr u64 size32 = sizeof(u32);

        u64 size = size32 + scene.name.size() + size32;

        for (auto& lod : scene.lodGroups)
        {
            u64 lodSize = size32 + lod.name.size() + size32;

            for (auto& m : lod.meshes) { lodSize += get_mesh_size(m); }
            size += lodSize;
        }

        return size;
    }

    void pack_mesh_data(const mesh& m, u8* const buffer, u64& at)
    {
        constexpr u64 size32 = sizeof(u32);
        u32 s = 0;

        // Mesh name
        s = (u32)m.name.size();
        memcpy(&buffer[at], &s, size32);
        at += size32;

        memcpy(&buffer[at], m.name.c_str(), s);
        at += s;

        // lod id
        s = m.lodId;
        memcpy(&buffer[at], &s, size32);
        at += size32;

        // vertex size
        constexpr u32 vertexSize = sizeof(packed_vertex::vertex_static);
        s = vertexSize;
        memcpy(&buffer[at], &s, size32);
        at += size32;

        // num verts
        const u32 numVerts = (u32)m.vertices.size();
        s = numVerts;
        memcpy(&buffer[at], &s, size32);
        at += size32;

        // Index Size
        const u32 indexSize = numVerts < 1 << 16 ? sizeof(u16) : sizeof(u32);
        s = indexSize;
        memcpy(&buffer[at], &s, size32);
        at += size32;

        // Num indices
        const u32 numIndices = (u32)m.indices.size();
        s = numIndices;
        memcpy(&buffer[at], &s, size32);
        at += size32;

        // lod threshold
        memcpy(&buffer[at], &m.lodThreshold, sizeof(f32));
        at += sizeof(f32);

        // Vertex data
        s = vertexSize * numVerts;
        memcpy(&buffer[at], m.packedVerticesStatic.data(), s);
        at += s;

        // Index data
        s = indexSize * numIndices;
        void* data = (void*)m.indices.data();
        utl::vector<u16> indices;
        if (indexSize == sizeof(u16))
        {
            indices.resize(numIndices);
            for (u32 i = 0; i < numIndices; ++i)
            {
                indices[i] = (u16)m.indices[i];
            }
            data = (void*)indices.data();
        }
        memcpy(&buffer[at], data, s);
        at += s;
    }

} // namespace

void process_scene(scene& scene, const geometry_import_settings& settings)
{
    for (auto& lod : scene.lodGroups)
    {
        for (auto& m : lod.meshes) { process_vertices(m, settings); }
    }
}



void pack_data(const scene& scene, scene_data& data)
{
    constexpr u64 size32    = sizeof(u32);
    const u64     sceneSize = get_scene_size(scene);
    data.bufferSize         = (u32) sceneSize;
    data.buffer             = (u8*) CoTaskMemAlloc(sceneSize);
    LASSERT(data.buffer);

    u8* const buffer = data.buffer;
    u64       at     = 0;
    u32       s      = 0;

    // scene name
    s = (u32) scene.name.size();
    memcpy(&buffer [ at ], &s, size32);
    at += size32;

    memcpy(&buffer[at], scene.name.c_str(), s);
    at += s;

    // Number of lods
    s = (u32)scene.lodGroups.size();
    memcpy(&buffer[at], &s, size32);
    at += size32;

    for(auto& lod : scene.lodGroups)
    {
        s = (u32)lod.name.size();
        memcpy(&buffer[at], &s, size32);
        at += size32;

        memcpy(&buffer[at], lod.name.c_str(), s);
        at += s;

        // Number of meshes in lod
        s = (u32)lod.meshes.size();
        memcpy(&buffer[at], &s, size32);
        at += size32;

        for(auto& m : lod.meshes)
        {
            pack_mesh_data(m, buffer, at);
        }
    }
}

} // namespace lotus::tools
