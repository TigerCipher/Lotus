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
// File Name: MeshPrimitives.h
// Date File Created: 9/10/2022
// Author: Matt
// 
// ------------------------------------------------------------------------------
#pragma once

#include "Common.h"

namespace lotus::tools
{

enum PrimitiveMeshType : uint32
{
    MESH_PLANE,
    MESH_CUBE,
    MESH_UV_SPHERE,
    MESH_ICO_SPHERE,
    MESH_CYLINDER,
    MESH_CAPSULE,

    COUNT
};

struct primitive_create_info
{
    PrimitiveMeshType type;
    uint32            segments [ 3 ] { 1, 1, 1 };
    vec3              size { 1, 1, 1 };
    uint32            lod = 0;
};

}