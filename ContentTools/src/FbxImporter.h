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
// File Name: FbxImporter.h
// Date File Created: 01/28/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#pragma once

#include "Common.h"
#include <fbxsdk.h>

namespace lotus::tools
{
struct scene_data;
struct scene;
struct mesh;
struct geometry_import_settings;

class fbx_context
{
public:
    fbx_context(const char* file, scene* scene, scene_data* data) : m_scene(scene), m_scene_data(data)
    {
        LASSERT(file && m_scene && m_scene_data);

        if (initialize_fbx())
        {
            load_fbx_file(file);
            LASSERT(is_valid());
        }
    }

    ~fbx_context()
    {
        m_fbx_scene->Destroy();
        m_fbx_manager->Destroy();
        ZeroMemory(this, sizeof(fbx_context));
    }

    void get_scene(FbxNode* root = nullptr) const;

    [[nodiscard]] constexpr bool is_valid() const { return m_fbx_manager && m_fbx_scene; }
    [[nodiscard]] constexpr f32  scene_scale() const { return m_scene_scale; }

private:
    bool initialize_fbx();
    void load_fbx_file(const char* file);
    bool get_mesh_data(FbxMesh* fbx_mesh, mesh& mesh) const;
    void get_meshes(FbxNode* node, utl::vector<mesh>& meshes, u32 lod_id, f32 lod_threshold) const;
    void get_mesh(FbxNodeAttribute* attrib, utl::vector<mesh>& meshes, u32 lod_id, f32 lod_threshold) const;
    void get_lod_group(FbxNodeAttribute* attrib) const;

    scene*      m_scene{ nullptr };
    scene_data* m_scene_data{ nullptr };
    FbxManager* m_fbx_manager{ nullptr };
    FbxScene*   m_fbx_scene{ nullptr };
    f32         m_scene_scale{ 1.0f };
};

} // namespace lotus::tools