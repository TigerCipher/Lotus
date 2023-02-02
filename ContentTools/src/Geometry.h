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


struct vertex
{
    vec4  tangent{};
    vec4  joint_weights{};
    vec4u joint_indices{ invalid_id_u32, invalid_id_u32, invalid_id_u32, invalid_id_u32 };
    vec3  position{};
    vec3  normal{};
    vec2  uv{};
    u8    red{};
    u8    green{};
    u8    blue{};
    u8    pad{};
};

namespace elements
{
struct elements_type
{
    enum type : u32
    {
        position_only                 = 0x00,
        static_normal                 = 0x01,
        static_normal_texture         = 0x03,
        static_color                  = 0x04,
        skeletal                      = 0x08,
        skeletal_color                = skeletal | static_color,
        skeletal_normal               = skeletal | static_normal,
        skeletal_normal_color         = skeletal_normal | static_color,
        skeletal_normal_texture       = skeletal | static_normal_texture,
        skeletal_normal_texture_color = skeletal_normal_texture | static_color,
    };
};


struct static_color
{
    u8 color[3];
    u8 pad;
};

struct static_normal
{
    u8  color[3];
    u8  tsign; // Bit 0: tangent handedness * (tangent.z sign), Bit 1: normal.z sign (0 means -1, 1 means +1)
    u16 normal[2];
};

struct static_normal_texture
{
    u8   color[3];
    u8   tsign; // Bit 0: tangent handedness * (tangent.z sign), Bit 1: normal.z sign (0 means -1, 1 means +1)
    u16  normal[2];
    u16  tangent[2];
    vec2 uv;
};

struct skeletal
{
    u8  joint_weights[3];
    u8  pad;
    u16 joint_indices[4];
};

struct skeletal_color
{
    u8  joint_weights[3];
    u8  pad;
    u16 joint_indices[4];
    u8  color[3];
    u8  pad2;
};

struct skeletal_normal
{
    u8  joint_weights[3];
    u8  tsign; // Bit 0: tangent handedness * (tangent.z sign), Bit 1: normal.z sign (0 means -1, 1 means +1)
    u16 joint_indices[4];
    u16 normal[2];
};

struct skeletal_normal_color
{
    u8  joint_weights[3];
    u8  tsign; // Bit 0: tangent handedness * (tangent.z sign), Bit 1: normal.z sign (0 means -1, 1 means +1)
    u16 joint_indices[4];
    u16 normal[2];
    u8  color[3];
    u8  pad;
};

struct skeletal_normal_texture
{
    u8   joint_weights[3];
    u8   tsign; // Bit 0: tangent handedness * (tangent.z sign), Bit 1: normal.z sign (0 means -1, 1 means +1)
    u16  joint_indices[4];
    u16  normal[2];
    u16  tangent[2];
    vec2 uv;
};

struct skeletal_normal_texture_color
{
    u8   joint_weights[3];
    u8   tsign; // Bit 0: tangent handedness * (tangent.z sign), Bit 1: normal.z sign (0 means -1, 1 means +1)
    u16  joint_indices[4];
    u16  normal[2];
    u16  tangent[2];
    vec2 uv;
    u8   color[3];
    u8   pad;
};

} // namespace elements

struct mesh
{
    utl::vector<vec3> positions;
    utl::vector<vec3> normals;
    utl::vector<vec4> tangents;
    utl::vector<vec3> colors;

    utl::vector<utl::vector<vec2>> uv_sets;

    utl::vector<u32> material_indices;
    utl::vector<u32> material_used;

    utl::vector<u32> raw_indices;

    utl::vector<vertex> vertices;
    utl::vector<u32>    indices;


    // output
    std::string                   name;
    elements::elements_type::type elements_type;
    utl::vector<u8>               position_buffer;
    utl::vector<u8>               element_buffer;

    f32 lod_threshold = -1.0f;
    u32 lod_id{ invalid_id_u32 };
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
