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

        lod_group lod{};
        get_meshes(node, lod.meshes, 0, -1.0f);
        if (lod.meshes.size())
        {
            lod.name = lod.meshes[0].name;
            m_scene->lod_groups.emplace_back(lod);
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
    FbxNode* const node = fbx_mesh->GetNode();
    FbxAMatrix     geometric_transform;
    geometric_transform.SetT(node->GetGeometricTranslation(FbxNode::eSourcePivot));
    geometric_transform.SetR(node->GetGeometricRotation(FbxNode::eSourcePivot));
    geometric_transform.SetS(node->GetGeometricScaling(FbxNode::eSourcePivot));

    const FbxAMatrix transform         = node->EvaluateGlobalTransform() * geometric_transform;
    const FbxAMatrix inverse_transpose = transform.Inverse().Transpose();

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
            FbxVector4 v        = transform.MultT(verts[v_idx]) * m_scene_scale;
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
                FbxVector4 n = inverse_transpose.MultT(normals[i]);
                n.Normalize();
                mesh.normals.emplace_back((f32) n[0], (f32) n[1], (f32) n[2]);
            }
        } else
        {
            // FBX normals messed up, fall back to Lotus calculations
            m_scene_data->settings.calculate_normals = true;
        }
    }

    // Tangents
    if (import_tangents)
    {
        FbxLayerElementArrayTemplate<FbxVector4>* tangents{ nullptr };

        // Use FBX to calculate tangents
        if (fbx_mesh->GenerateTangentsData() && fbx_mesh->GetTangents(&tangents) && tangents && tangents->GetCount() > 0)
        {
            const i32 num_tangents = tangents->GetCount();
            for (i32 i = 0; i < num_tangents; ++i)
            {
                FbxVector4 t          = tangents->GetAt(i);
                const f32  handedness = (f32) t[3];
                t[3]                  = 0.0;
                t = transform.MultT(t);
                t.Normalize();
                mesh.tangents.emplace_back((f32) t[0], (f32) t[1], (f32) t[2], handedness);
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
void fbx_context::get_meshes(FbxNode* node, utl::vector<mesh>& meshes, u32 lod_id, f32 lod_threshold) const
{
    LASSERT(node && lod_id != invalid_id_u32);
    bool is_lodgroup = false;

    if (const i32 num_attribs = node->GetNodeAttributeCount())
    {
        for (i32 i = 0; i < num_attribs; ++i)
        {
            FbxNodeAttribute*             attrib      = node->GetNodeAttributeByIndex(i);
            const FbxNodeAttribute::EType attrib_type = attrib->GetAttributeType();
            if (attrib_type == FbxNodeAttribute::eMesh)
            {
                get_mesh(attrib, meshes, lod_id, lod_threshold);
            } else if (attrib_type == FbxNodeAttribute::eLODGroup)
            {
                get_lod_group(attrib);
                is_lodgroup = true;
            }
        }
    }

    if (!is_lodgroup)
    {
        if (const i32 num_children = node->GetChildCount())
        {
            for (i32 i = 0; i < num_children; ++i)
            {
                get_meshes(node->GetChild(i), meshes, lod_id, lod_threshold);
            }
        }
    }
}

void fbx_context::get_mesh(FbxNodeAttribute* attrib, utl::vector<mesh>& meshes, u32 lod_id, f32 lod_threshold) const
{
    LASSERT(attrib);

    FbxMesh* fbx_mesh = (FbxMesh*) attrib;
    if (fbx_mesh->RemoveBadPolygons() < 0)
        return;

    FbxGeometryConverter gc{ m_fbx_manager };
    fbx_mesh = (FbxMesh*) gc.Triangulate(fbx_mesh, true);
    if (!fbx_mesh || fbx_mesh->RemoveBadPolygons() < 0)
        return;

    FbxNode* const node = fbx_mesh->GetNode();
    mesh           m{};
    m.lod_id        = lod_id;
    m.lod_threshold = lod_threshold;
    m.name          = node->GetName()[0] != '\0' ? node->GetName() : fbx_mesh->GetName();

    if (get_mesh_data(fbx_mesh, m))
    {
        meshes.emplace_back(m);
    }
}

void fbx_context::get_lod_group(FbxNodeAttribute* attrib) const
{
    LASSERT(attrib);

    const auto*    lodgrp = (FbxLODGroup*) attrib;
    FbxNode* const node   = lodgrp->GetNode();
    lod_group      lod{};
    lod.name = node->GetName()[0] != '\0' ? node->GetName() : lodgrp->GetName();

    const i32 num_nodes = node->GetChildCount();

    LASSERT(num_nodes > 0 && lodgrp->GetNumThresholds() == (num_nodes - 1));

    for (i32 i = 0; i < num_nodes; ++i)
    {
        f32 lod_threshold = -1.0f;
        if (i > 0)
        {
            FbxDistance threshold;
            lodgrp->GetThreshold(i - 1, threshold);
            lod_threshold = threshold.value() * m_scene_scale;
        }

        get_meshes(node->GetChild(i), lod.meshes, (u32) lod.meshes.size(), lod_threshold);
    }

    if (lod.meshes.size())
    {
        m_scene->lod_groups.emplace_back(lod);
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