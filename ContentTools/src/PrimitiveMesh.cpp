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

    void create_plane(scene& scene, const primitive_create_info& info);
    void create_cube(scene& scene, const primitive_create_info& info);
    void create_uv_sphere(scene& scene, const primitive_create_info& info);
    void create_ico_sphere(scene& scene, const primitive_create_info& info);
    void create_cylinder(scene& scene, const primitive_create_info& info);
    void create_capsule(scene& scene, const primitive_create_info& info);

    PrimitiveMeshCreator creators [] { create_plane,      create_cube,     create_uv_sphere,
                                       create_ico_sphere, create_cylinder, create_capsule };

    static_assert(_countof(creators) == PrimitiveMeshType::COUNT);
} // namespace

EDITOR_INTERFACE void CreatePrimitiveMesh(scene_data* data, primitive_create_info* info)
{
    LASSERT(data && info);
    LASSERT(info->type < PrimitiveMeshType::COUNT);

    scene scene;
}

} // namespace lotus::tools
