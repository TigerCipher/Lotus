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
using primitive_mesh_creator = void (*)(scene&, const primitive_create_info& info);


struct axis
{
    enum : u32
    {
        x = 0,
        y = 1,
        z = 2
    };
};

void create_plane(scene& scene, const primitive_create_info& info);
void create_cube(scene& scene, const primitive_create_info& info);
void create_uv_sphere(scene& scene, const primitive_create_info& info);
void create_ico_sphere(scene& scene, const primitive_create_info& info);
void create_cylinder(scene& scene, const primitive_create_info& info);
void create_capsule(scene& scene, const primitive_create_info& info);

primitive_mesh_creator creators[]{ create_plane,      create_cube,     create_uv_sphere,
                                   create_ico_sphere, create_cylinder, create_capsule };

static_assert(_countof(creators) == primitive_mesh_type::MESH_COUNT);

mesh create_plane(const primitive_create_info& info, const u32 horizontal_index = axis::x,
                  const u32 vertical_index = axis::z, const bool flip_winding = false,
                  const vec3 offset = { -0.5f, 0.0f, -0.5f }, const vec2 u_range = { 0.0f, 1.0f },
                  const vec2 v_range = { 0.0f, 1.0f })
{
    assert(horizontal_index < 3 && vertical_index < 3 && horizontal_index != vertical_index);
    const u32 horiz_count = math::clamp(info.segments[horizontal_index], 1u, 10u);
    const u32 vert_count  = math::clamp(info.segments[vertical_index], 1u, 10u);
    const f32 horiz_step  = 1.0f / horiz_count;
    const f32 vert_step   = 1.0f / vert_count;
    const f32 u_step      = (u_range.y - u_range.x) / horiz_count;
    const f32 v_step      = (v_range.y - v_range.x) / vert_count;

    mesh              m;
    utl::vector<vec2> uvs;

    for (u32 j = 0; j <= vert_count; ++j)
    {
        for (u32 i = 0; i <= horiz_count; ++i)
        {
            vec3       pos = offset;
            f32* const arr = &pos.x;
            arr[horizontal_index] += i * horiz_step;
            arr[vertical_index] += j * vert_step;
            m.positions.emplace_back(pos.x * info.size.x, pos.y * info.size.y, pos.z * info.size.z);

            vec2 uv = { u_range.x, 1.0f - v_range.x };
            uv.x += i * u_step;
            uv.y -= j * v_step;
            // vec2 uv{ 0, 1.0f };
            // uv.x += i % 2;
            // uv.y -= j % 2;

            uvs.emplace_back(uv);
        }
    }

    assert(m.positions.size() == ((u64) horiz_count + 1) * ((u64) vert_count + 1));

    const u32 row_len = horiz_count + 1;

    for (u32 j = 0; j < vert_count; ++j)
    {
        for (u32 i = 0; i < horiz_count; ++i)
        {
            const u32 index[4]{
                i + j * row_len,
                i + (j + 1) * row_len,
                (i + 1) + j * row_len,
                (i + 1) + (j + 1) * row_len,
            };

            m.raw_indices.emplace_back(index[0]);
            m.raw_indices.emplace_back(index[flip_winding ? 2 : 1]);
            m.raw_indices.emplace_back(index[flip_winding ? 1 : 2]);

            m.raw_indices.emplace_back(index[2]);
            m.raw_indices.emplace_back(index[flip_winding ? 3 : 1]);
            m.raw_indices.emplace_back(index[flip_winding ? 1 : 3]);
        }
    }

    const u32 num_indices = 3 * 2 * horiz_count * vert_count;
    assert(m.raw_indices.size() == num_indices);

    m.uv_sets.resize(1);
    for (u32 i = 0; i < num_indices; ++i)
    {
        m.uv_sets[0].emplace_back(uvs[m.raw_indices[i]]);
    }

    return m;
}

mesh create_uv_sphere(const primitive_create_info& info)
{
    const u32 phi_count   = math::clamp(info.segments[axis::x], 3u, 64u);
    const u32 theta_count = math::clamp(info.segments[axis::y], 2u, 64u);
    const f32 theta_step  = math::pi / theta_count;
    const f32 phi_step    = math::two_pi / phi_count;
    const u32 num_verts   = 2 + phi_count * (theta_count - 1);
    const u32 num_indices = 2 * 3 * phi_count + 2 * 3 * phi_count * (theta_count - 2);

    mesh m;
    m.name = "uv_sphere";
    m.positions.resize(num_verts);
    m.raw_indices.resize(num_indices);

    // Top vertex
    u32 c            = 0;
    m.positions[c++] = { 0.0f, info.size.y, 0.0f };

    for (u32 j = 1; j <= theta_count - 1; ++j)
    {
        const f32 theta = (f32) j * theta_step;
        for (u32 i = 0; i < phi_count; ++i)
        {
            const f32 phi    = (f32) i * phi_step;
            m.positions[c++] = { info.size.x * math::scalar_sin(theta) * math::scalar_cos(phi),
                                 info.size.y * math::scalar_cos(theta),
                                 -info.size.z * math::scalar_sin(theta) * math::scalar_sin(phi) };
        }
    }

    // Bottom vertex
    m.positions[c++] = { 0.0f, -info.size.y, 0.0f };

    assert(c == num_verts);

    c = 0;

    utl::vector<vec2> uvs(num_indices);
    const f32         inv_theta_count = 1.0f / theta_count;
    const f32         inv_phi_count   = 1.0f / phi_count;

    // top cap indices
    for (u32 i = 0; i < phi_count - 1; ++i)
    {
        uvs[c]             = { (2 * i + 1) * 0.5f * inv_phi_count, 1.0f };
        m.raw_indices[c++] = 0;
        uvs[c]             = { i * inv_phi_count, 1.0f - inv_theta_count };
        m.raw_indices[c++] = i + 1;
        uvs[c]             = { (i + 1) * inv_phi_count, 1.0f - inv_theta_count };
        m.raw_indices[c++] = i + 2;
    }

    uvs[c]             = { 1.0f - 0.5f * inv_phi_count, 1.0f };
    m.raw_indices[c++] = 0;
    uvs[c]             = { 1.0f - inv_phi_count, 1.0f - inv_theta_count };
    m.raw_indices[c++] = phi_count;
    uvs[c]             = { 1.0f, 1.0f - inv_theta_count };
    m.raw_indices[c++] = 1;

    // Mid section
    for (u32 j = 0; j < theta_count - 2; ++j)
    {
        for (u32 i = 0; i < phi_count - 1; ++i)
        {
            const u32 index[4] = {
                1 + i + j * phi_count,             //
                1 + i + (j + 1) * phi_count,       //
                1 + (i + 1) + (j + 1) * phi_count, //
                1 + (i + 1) + j * phi_count        //
            };

            uvs[c]             = { i * inv_phi_count, 1.0f - (j + 1) * inv_theta_count };
            m.raw_indices[c++] = index[0];
            uvs[c]             = { i * inv_phi_count, 1.0f - (j + 2) * inv_theta_count };
            m.raw_indices[c++] = index[1];
            uvs[c]             = { (i + 1) * inv_phi_count, 1.0f - (j + 2) * inv_theta_count };
            m.raw_indices[c++] = index[2];

            uvs[c]             = { i * inv_phi_count, 1.0f - (j + 1) * inv_theta_count };
            m.raw_indices[c++] = index[0];
            uvs[c]             = { (i + 1) * inv_phi_count, 1.0f - (j + 2) * inv_theta_count };
            m.raw_indices[c++] = index[2];
            uvs[c]             = { (i + 1) * inv_phi_count, 1.0f - (j + 1) * inv_theta_count };
            m.raw_indices[c++] = index[3];
        }

        const u32 index[4] = {
            phi_count + j * phi_count,       //
            phi_count + (j + 1) * phi_count, //
            1 + (j + 1) * phi_count,         //
            1 + j * phi_count                //
        };

        uvs[c]             = { 1.0f - inv_phi_count, 1.0f - (j + 1) * inv_theta_count };
        m.raw_indices[c++] = index[0];
        uvs[c]             = { 1.0f - inv_phi_count, 1.0f - (j + 2) * inv_theta_count };
        m.raw_indices[c++] = index[1];
        uvs[c]             = { 1.0f, 1.0f - (j + 2) * inv_theta_count };
        m.raw_indices[c++] = index[2];

        uvs[c]             = { 1.0f - inv_phi_count, 1.0f - (j + 1) * inv_theta_count };
        m.raw_indices[c++] = index[0];
        uvs[c]             = { 1.0f, 1.0f - (j + 2) * inv_theta_count };
        m.raw_indices[c++] = index[2];
        uvs[c]             = { 1.0f, 1.0f - (j + 1) * inv_theta_count };
        m.raw_indices[c++] = index[3];
    }

    // Bottom cap
    const u32 south_index = (u32) m.positions.size() - 1;
    for (u32 i = 0; i < phi_count - 1; ++i)
    {
        uvs[c]             = { (2 * i + 1) * 0.5f * inv_phi_count, 0.0f };
        m.raw_indices[c++] = south_index;
        uvs[c]             = { (i + 1) * inv_phi_count, inv_theta_count };
        m.raw_indices[c++] = south_index - phi_count + i + 1;
        uvs[c]             = { i * inv_phi_count, inv_theta_count };
        m.raw_indices[c++] = south_index - phi_count + i;
    }

    uvs[c]             = { 1.0f - 0.5f * inv_phi_count, 0.0f };
    m.raw_indices[c++] = south_index;
    uvs[c]             = { 1.0f, inv_theta_count };
    m.raw_indices[c++] = south_index - phi_count;
    uvs[c]             = { 1.0f - inv_phi_count, inv_theta_count };
    m.raw_indices[c++] = south_index - 1;

    assert(c == num_indices);

    m.uv_sets.emplace_back(uvs);

    return m;
}

void create_plane(scene& scene, const primitive_create_info& info)
{
    lod_group lod{ "plane" };
    lod.meshes.emplace_back(create_plane(info));
    scene.lod_groups.emplace_back(lod);
}
void create_cube([[maybe_unused]] scene& scene, [[maybe_unused]] const primitive_create_info& info) {}

void create_uv_sphere(scene& scene, const primitive_create_info& info)
{
    lod_group lod{ "uv_sphere" };
    lod.meshes.emplace_back(create_uv_sphere(info));
    scene.lod_groups.emplace_back(lod);
}

void create_ico_sphere([[maybe_unused]] scene& scene, [[maybe_unused]] const primitive_create_info& info) {}
void create_cylinder([[maybe_unused]] scene& scene, [[maybe_unused]] const primitive_create_info& info) {}
void create_capsule([[maybe_unused]] scene& scene, [[maybe_unused]] const primitive_create_info& info) {}

} // namespace

EDITOR_INTERFACE void CreatePrimitiveMesh(scene_data* data, primitive_create_info* info)
{
    assert(data && info);
    assert(info->type < primitive_mesh_type::MESH_COUNT);

    scene scene;
    creators[info->type](scene, *info);

    data->settings.calculate_normals = 1;
    process_scene(scene, data->settings);
    pack_data(scene, *data);
}

} // namespace lotus::tools
