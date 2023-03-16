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
// File Name: Transform.cpp
// Date File Created: 8/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "Transform.h"

namespace lotus::transform
{

namespace
{
utl::vector<mat4> to_world;
utl::vector<mat4> inv_world;
utl::vector<vec4> rotations;
utl::vector<vec3> orientations;
utl::vector<vec3> positions;
utl::vector<vec3> scales;
utl::vector<u8>   has_transform;

vec3 calculate_orientation(vec4 rotation)
{
    const vec rotation_quat = math::load_float4(&rotation);
    const vec front         = math::set_vector(0.0f, 0.0f, 1.0f, 0.0f);
    vec3      orientation;
    math::store_float3(&orientation, math::rotate_vec3(front, rotation_quat));

    return orientation;
}


void calculate_transform_matrices(id::id_type index)
{
    assert(rotations.size() >= index);
    assert(positions.size() >= index);
    assert(scales.size() >= index);

    using namespace DirectX;

    const vec r{ XMLoadFloat4(&rotations[index]) };
    const vec t{ XMLoadFloat3(&positions[index]) };
    const vec s{ XMLoadFloat3(&scales[index]) };

    mat world{ XMMatrixAffineTransformation(s, XMQuaternionIdentity(), r, t) };
    XMStoreFloat4x4(&to_world[index], world);
    world.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

    const mat inverse_world{ XMMatrixInverse(nullptr, world) };
    XMStoreFloat4x4(&inv_world[index], inverse_world);

    has_transform[index] = 1;
}

} // anonymous namespace

component create(const create_info& info, game_entity::entity entity)
{
    assert(entity.is_valid());

    if (const id::id_type ent_index = id::index(entity.get_id()); positions.size() > ent_index)
    {
        const vec4 rotation{ info.rotation };
        rotations[ent_index]     = rotation;
        orientations[ent_index]  = calculate_orientation(rotation);
        positions[ent_index]     = vec3{ info.position };
        scales[ent_index]        = vec3{ info.scale };
        has_transform[ent_index] = 0;
    } else
    {
        assert(positions.size() == ent_index);
        to_world.emplace_back();
        inv_world.emplace_back();
        rotations.emplace_back(info.rotation);
        orientations.emplace_back(calculate_orientation(vec4{ info.rotation }));
        positions.emplace_back(info.position);
        scales.emplace_back(info.scale);
        has_transform.emplace_back((u8) 0);
    }

    // returns the entity ID because since every entity has a transform component, the ids are the same
    return component(transform_id{ entity.get_id() });
}

void remove([[maybe_unused]] const component comp)
{
    assert(comp.is_valid());
}


void get_transform_matrices(const game_entity::entity_id id, mat4& world, mat4& inverse_world)
{
    assert(game_entity::entity{ id }.is_valid());

    const id::id_type ent_idx{ id::index(id) };
    if (!has_transform[ent_idx])
    {
        calculate_transform_matrices(ent_idx);
    }

    world         = to_world[ent_idx];
    inverse_world = inv_world[ent_idx];
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TransformComponent Class Implementations ////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


vec3 component::position() const
{
    assert(is_valid());
    return positions[id::index(m_id)];
}
vec4 component::rotation() const
{
    assert(is_valid());
    return rotations[id::index(m_id)];
}
vec3 component::scale() const
{
    assert(is_valid());
    return scales[id::index(m_id)];
}

vec3 component::orientation() const
{
    assert(is_valid());
    return orientations[id::index(m_id)];
}


} // namespace lotus::transform
