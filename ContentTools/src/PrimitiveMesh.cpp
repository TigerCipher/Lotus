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
// File Name: MeshPrimitives.cpp
// Date File Created: 9/10/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "PrimitiveMesh.h"
#include "Geometry.h"

namespace lotus::tools
{

namespace
{
    using PrimitiveMeshCreator = void (*)(scene&, const primitive_create_info& info);


    enum Axis : u32
    {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_Z = 2
    };

    void create_plane(scene& scene, const primitive_create_info& info);
    void create_cube(scene& scene, const primitive_create_info& info);
    void create_uv_sphere(scene& scene, const primitive_create_info& info);
    void create_ico_sphere(scene& scene, const primitive_create_info& info);
    void create_cylinder(scene& scene, const primitive_create_info& info);
    void create_capsule(scene& scene, const primitive_create_info& info);

    PrimitiveMeshCreator creators [] { create_plane,      create_cube,     create_uv_sphere,
                                       create_ico_sphere, create_cylinder, create_capsule };

    static_assert(_countof(creators) == PrimitiveMeshType::COUNT);

    mesh create_plane(const primitive_create_info& info, u32 horizontalIndex = AXIS_X, u32 verticalIndex = AXIS_Z,
                      bool flipWinding = false, vec3 offset = { -0.5f, 0.0f, -0.5f }, vec2 uRange = { 0.0f, 1.0f },
                      vec2 vRange = { 0.0f, 1.0f })
    {
        LASSERT(horizontalIndex < 3 && verticalIndex < 3 && horizontalIndex != verticalIndex);
        const u32 horizCount = math::clamp(info.segments [ horizontalIndex ], 1u, 10u);
        const u32 vertCount  = math::clamp(info.segments [ verticalIndex ], 1u, 10u);
        const f32 horizStep  = 1.0f / horizCount;
        const f32 vertStep   = 1.0f / vertCount;
        const f32 uStep      = (uRange.y - uRange.x) / horizCount;
        const f32 vStep      = (vRange.y - vRange.x) / vertCount;

        mesh              m;
        utl::vector<vec2> uvs;

        for (u32 j = 0; j <= vertCount; ++j)
        {
            for (u32 i = 0; i <= horizCount; ++i)
            {
                vec3       pos = offset;
                f32* const arr = &pos.x;
                arr [ horizontalIndex ] += i * horizStep;
                arr [ verticalIndex ] += j * vertStep;
                m.positions.emplace_back(pos.x * info.size.x, pos.y * info.size.y, pos.z * info.size.z);

                vec2 uv = { uRange.x, 1.0f - vRange.x };
                uv.x += i * uStep;
                uv.y -= j * vStep;
                uvs.emplace_back(uv);
            }
        }

        LASSERT(m.positions.size() == ((u64) horizCount + 1) * ((u64) vertCount + 1));

        const u32 rowLen = horizCount + 1;

        for (u32 j = 0; j < vertCount; ++j)
        {
            for (u32 i = j; i < horizCount; ++i)
            {
                const u32 index [ 4 ] {
                    i + j * rowLen,
                    i + (j + 1) * rowLen,
                    (i + 1) + j * rowLen,
                    (i + 1) + (j + 1) * rowLen,
                };

                m.rawIndices.emplace_back(index [ 0 ]);
                m.rawIndices.emplace_back(index [ flipWinding ? 2 : 1 ]);
                m.rawIndices.emplace_back(index [ flipWinding ? 1 : 2 ]);

                m.rawIndices.emplace_back(index [ 2 ]);
                m.rawIndices.emplace_back(index [ flipWinding ? 3 : 1 ]);
                m.rawIndices.emplace_back(index [ flipWinding ? 1 : 3 ]);
            }
        }

        const u32 numIndices = 3 * 2 * horizCount * vertCount;
        LASSERT(m.rawIndices.size() == numIndices);

        for (u32 i = 0; i < numIndices; ++i) { m.uvSets [ 0 ].emplace_back(uvs [ i ]); }

        return m;
    }

    void create_plane(scene& scene, const primitive_create_info& info)
    {
        lod_group lod { "plane" };
        lod.meshes.emplace_back(create_plane(info));
        scene.lodGroups.emplace_back(lod);
    }
    void create_cube(scene& scene, const primitive_create_info& info) { }
    void create_uv_sphere(scene& scene, const primitive_create_info& info) { }
    void create_ico_sphere(scene& scene, const primitive_create_info& info) { }
    void create_cylinder(scene& scene, const primitive_create_info& info) { }
    void create_capsule(scene& scene, const primitive_create_info& info) { }

} // namespace

EDITOR_INTERFACE void CreatePrimitiveMesh(scene_data* data, primitive_create_info* info)
{
    LASSERT(data && info);
    LASSERT(info->type < PrimitiveMeshType::COUNT);

    scene scene;
    creators [ info->type ](scene, *info);

    data->settings.calculateNormals = 1;
    process_scene(scene, data->settings);
    pack_data(scene, *data);
}

} // namespace lotus::tools
