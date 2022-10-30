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

namespace packed_vertex
{
// static mesh
struct vertex_static
{
    vec3 position;
    u8   reserved[3];
    u8   tsign; // if z is neg: Bit 1 = 0, if pos: Bit 1 = 1
    u16  normal[2];
    u16  tangent[2];
    vec2 uv;
};
} // namespace packed_vertex

struct vertex
{
    vec4 tangent;
    vec3 position;
    vec3 normal;
    vec2 uv;
};

struct mesh
{
    utl::vector<vec3>              positions;
    utl::vector<vec3>              normals;
    utl::vector<vec4>              tangents;
    utl::vector<utl::vector<vec2>> uv_sets;
    utl::vector<u32>               raw_indices;

    utl::vector<vertex> vertices;
    utl::vector<u32>    indices;


    // output
    std::string                               name;
    utl::vector<packed_vertex::vertex_static> packed_vertices_static;
    f32                                       lod_threshold = -1.0f;
    u32                                       lod_id{InvalidIdU32};
};

struct lod_group
{
    std::string       name;
    utl::vector<mesh> meshes;
};

struct scene
{
    std::string            name;
    utl::vector<lod_group> lod_groups;
};

struct geometry_import_settings
{
    f32 smoothing_angle;
    u8  calculate_normals;
    u8  calculate_tangents;
    u8  reverse_handedness;
    u8  import_embeded_textures;
    u8  import_animations;
};

struct scene_data
{
    u8* buffer;
    u32 buffer_size;

    geometry_import_settings settings;
};

void process_scene(scene& scene, const geometry_import_settings& settings);
void pack_data(const scene& scene, scene_data& data);

} // namespace lotus::tools
