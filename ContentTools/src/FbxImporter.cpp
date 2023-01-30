// ------------------------------------------------------------------------------
//
// Lotus
//    Copyright 2023 Matthew Rogers
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
// File Name: FbxImporter.cpp
// Date File Created: 01/28/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "FbxImporter.h"
#include "Geometry.h"


#if L_DEBUG
    #pragma comment(lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.2\\lib\\vs2019\\x64\\debug\\libfbxsdk-md.lib")
    #pragma comment(lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.2\\lib\\vs2019\\x64\\debug\\libxml2-md.lib")
    #pragma comment(lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.2\\lib\\vs2019\\x64\\debug\\zlib-md.lib")
#else
    #pragma comment(lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.2\\lib\\vs2019\\x64\\release\\libfbxsdk-md.lib")
    #pragma comment(lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.2\\lib\\vs2019\\x64\\release\\libxml2-md.lib")
    #pragma comment(lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.2\\lib\\vs2019\\x64\\release\\zlib-md.lib")
#endif

namespace lotus::tools
{

namespace
{
std::mutex fbx_mutex;
} // anonymous namespace

void fbx_context::get_scene(FbxNode* root) const
{
    LASSERT(is_valid());

    if (!root)
    {
        root = m_fbx_scene->GetRootNode();
        if (!root)
            return;
    }

    const i32 num_nodes = root->GetChildCount();
    for (i32 i = 0; i < num_nodes; ++i)
    {
        FbxNode* node = root->GetChild(i);
        if (!node)
            continue;

        if (node->GetMesh())
        {
            lod_group lod{};
            get_mesh(node, lod.meshes);
            if (lod.meshes.size())
            {
                lod.name = lod.meshes[0].name;
                m_scene->lod_groups.emplace_back(lod);
            }
        } else if (node->GetLodGroup())
        {
            get_lod_group(node);
        } else
        {
            get_scene(node);
        }
    }
}

bool fbx_context::initialize_fbx()
{
    LASSERT(!is_valid());
    m_fbx_manager = FbxManager::Create();
    if (!m_fbx_manager)
        return false;

    FbxIOSettings* ios{ FbxIOSettings::Create(m_fbx_manager, IOSROOT) };
    LASSERT(ios);
    m_fbx_manager->SetIOSettings(ios);

    return true;
}

void fbx_context::load_fbx_file(const char* file)
{
    LASSERT(m_fbx_manager && !m_fbx_scene);
    m_fbx_scene = FbxScene::Create(m_fbx_manager, "Importer Scene");

    if (!m_fbx_scene)
        return;

    FbxImporter* importer{ FbxImporter::Create(m_fbx_manager, "Importer") };

    if (!(importer && importer->Initialize(file, -1, m_fbx_manager->GetIOSettings()) && importer->Import(m_fbx_scene)))
        return;
    importer->Destroy();

    // scale in meters
    m_scene_scale = (f32) m_fbx_scene->GetGlobalSettings().GetSystemUnit().GetConversionFactorTo(FbxSystemUnit::m);
}

bool fbx_context::get_mesh_data(FbxMesh* fbx_mesh, mesh& mesh) const
{
    LASSERT(fbx_mesh);
    const i32 num_polys = fbx_mesh->GetPolygonCount();
    if (num_polys <= 0)
        return false;

    const i32         num_verts = fbx_mesh->GetControlPointsCount();
    const FbxVector4* verts     = fbx_mesh->GetControlPoints();

    const i32  num_indices = fbx_mesh->GetPolygonVertexCount();
    const i32* indices     = fbx_mesh->GetPolygonVertices();

    LASSERT(num_verts > 0 && num_indices > 0 && verts && indices);

    if (!(num_verts > 0 && num_indices > 0 && verts && indices))
        return false;

    mesh.raw_indices.resize(num_indices);
    utl::vector vert_ref(num_verts, invalid_id_u32);

    for (i32 i = 0; i < num_indices; ++i)
    {
        const u32 v_idx = indices[i];

        if (vert_ref[v_idx] != invalid_id_u32)
        {
            mesh.raw_indices[i] = vert_ref[v_idx];
        } else
        {
            FbxVector4 v        = verts[v_idx] * m_scene_scale;
            mesh.raw_indices[i] = (u32) mesh.positions.size();
            vert_ref[v_idx]     = mesh.raw_indices[i];
            mesh.positions.emplace_back((f32) v[0], (f32) v[1], (f32) v[2]);
        }
    }

    LASSERT(mesh.raw_indices.size() % 3 == 0);

    // Get materials per polygon
    LASSERT(num_polys > 0);
    FbxLayerElementArrayTemplate<i32>* mtl_indices;
    if (fbx_mesh->GetMaterialIndices(&mtl_indices))
    {
        for (i32 i = 0; i < num_polys; ++i)
        {
            const i32 mtl_idx = mtl_indices->GetAt(i);
            mesh.material_indices.emplace_back((u32) mtl_idx);
            if (std::ranges::find(mesh.material_used, (u32) mtl_idx) == mesh.material_used.end())
            {
                mesh.material_used.emplace_back((u32) mtl_idx);
            }
        }
    }

    const bool import_normals  = !m_scene_data->settings.calculate_normals;
    const bool import_tangents = !m_scene_data->settings.calculate_tangents;

    // Normals
    if (import_normals)
    {
        FbxArray<FbxVector4> normals;
        // Use FBX to calculate normals
        if (fbx_mesh->GenerateNormals() && fbx_mesh->GetPolygonVertexNormals(normals) && normals.Size() > 0)
        {
            const i32 num_normals = normals.Size();
            for (i32 i = 0; i < num_normals; ++i)
            {
                mesh.normals.emplace_back((f32) normals[i][0], (f32) normals[i][1], (f32) normals[i][2]);
            }
        } else
        {
            // FBX normals messed up, fall back to Lotus calculations
            m_scene_data->settings.calculate_normals = true;
        }
    }

    if (import_tangents)
    {
        FbxLayerElementArrayTemplate<FbxVector4>* tangents{ nullptr };

        // Use FBX to calculate tangents
        if (fbx_mesh->GenerateTangentsData() && fbx_mesh->GetTangents(&tangents) && tangents && tangents->GetCount() > 0)
        {
            const i32 num_tangents = tangents->GetCount();
            for (i32 i = 0; i < num_tangents; ++i)
            {
                FbxVector4 t = tangents->GetAt(i);
                mesh.tangents.emplace_back((f32) t[0], (f32) t[1], (f32) t[2], (f32) t[3]);
            }
        } else
        {
            // FBX tangents messed up, fall back to Lotus calculations
            m_scene_data->settings.calculate_tangents = true;
        }
    }

    // UVs

    FbxStringList uv_names;
    fbx_mesh->GetUVSetNames(uv_names);
    const i32 uv_set_count = uv_names.GetCount();
    mesh.uv_sets.resize(uv_set_count);

    for (i32 i = 0; i < uv_set_count; ++i)
    {
        FbxArray<FbxVector2> uvs;
        if (fbx_mesh->GetPolygonVertexUVs(uv_names.GetStringAt(i), uvs))
        {
            const i32 num_uvs = uvs.Size();
            for (i32 j = 0; j < num_uvs; ++j)
            {
                mesh.uv_sets[i].emplace_back((f32) uvs[j][0], (f32) uvs[j][1]);
            }
        }
    }

    return true;
}

void fbx_context::get_mesh(FbxNode* node, utl::vector<mesh>& meshes) const
{
    LASSERT(node);

    if (FbxMesh * fbx_mesh{ node->GetMesh() })
    {
        if (fbx_mesh->RemoveBadPolygons() < 0)
            return;

        FbxGeometryConverter gc{ m_fbx_manager };
        fbx_mesh = (FbxMesh*) gc.Triangulate(fbx_mesh, true);
        if (!fbx_mesh || fbx_mesh->RemoveBadPolygons() < 0)
            return;

        mesh m{};
        m.lod_id        = (u32) meshes.size();
        m.lod_threshold = -1;
        m.name          = node->GetName()[0] != '\0' ? node->GetName() : fbx_mesh->GetName();

        if (get_mesh_data(fbx_mesh, m))
        {
            meshes.emplace_back(m);
        }
    }

    get_scene(node);
}

void fbx_context::get_lod_group(FbxNode* node) const
{
    LASSERT(node);

    if (FbxLODGroup* lodgrp = node->GetLodGroup())
    {
        lod_group lod{};
        lod.name = node->GetName()[0] != '\0' ? node->GetName() : lodgrp->GetName();

        const i32 num_lods  = lodgrp->GetNumThresholds();
        const i32 num_nodes = node->GetChildCount();

        for (i32 i = 0; i < num_nodes; ++i)
        {
            get_mesh(node->GetChild(i), lod.meshes);
            if (lod.meshes.size() > 1 && lod.meshes.size() <= num_lods + 1 && lod.meshes.back().lod_threshold < 0.0f)
            {
                FbxDistance threshold;
                lodgrp->GetThreshold((u32) lod.meshes.size() - 2, threshold);
                lod.meshes.back().lod_threshold = threshold.value() * m_scene_scale;
            }
        }

        if (lod.meshes.size())
        {
            m_scene->lod_groups.emplace_back(lod);
        }
    }
}

// ReSharper disable once CppInconsistentNaming
EDITOR_INTERFACE void ImportFbx(const char* file, scene_data* data)
{
    LASSERT(file && data);
    scene scene{};

    // FBX Can only be done on a single thread
    {
        std::lock_guard lock{ fbx_mutex };

        fbx_context fbx_context{ file, &scene, data };
        if (fbx_context.is_valid())
        {
            fbx_context.get_scene();
        } else
        {
            // TODO: Log error
            return;
        }
    }

    process_scene(scene, data->settings);
    pack_data(scene, *data);
}

} // namespace lotus::tools