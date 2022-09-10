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
// File Name: Geometry.h
// Date File Created: 9/10/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "Common.h"

namespace lotus::tools
{

struct mesh
{
    utl::vector<vec3> positions;
    utl::vector<vec3> normals;
    utl::vector<vec4> tangents;

    utl::vector<utl::vector<vec2>> uvSets;

    utl::vector<u32> rawIndices;
};

struct lod_group
{
    std::string       name;
    utl::vector<mesh> meshes;
};

struct scene
{
    std::string            name;
    utl::vector<lod_group> lodGroups;
};

struct geometry_import_settings
{
    f32 smoothingAngle;
    u8  calculateNormals;
    u8  calculateTangents;
    u8  reverseHandedness;
    u8  importEmbededTextures;
    u8  importAnimations;
};

struct scene_data
{
    u8* buffer;
    u32 bufferSize;

    geometry_import_settings settings;
};

} // namespace lotus::tools
