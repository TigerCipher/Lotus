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
utl::vector<u8>   changes_from_previous_frame;
u8                read_write_flag;

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

void set_rotation(transform_id id, const vec4& rotation_quaternion)
{
    const u32 index{ id::index(id) };
    rotations[index]     = rotation_quaternion;
    orientations[index]  = calculate_orientation(rotation_quaternion);
    has_transform[index] = 0;
    changes_from_previous_frame[index] |= component_flags::rotation;
}

void set_orientation(transform_id, const vec3&) {}

void set_position(transform_id id, const vec3& position)
{
    const u32 index{ id::index(id) };
    positions[index]     = position;
    has_transform[index] = 0;
    changes_from_previous_frame[index] |= component_flags::position;
}

void set_scale(transform_id id, const vec3& scale)
{
    const u32 index{ id::index(id) };
    scales[index]        = scale;
    has_transform[index] = 0;
    changes_from_previous_frame[index] |= component_flags::scale;
}


} // anonymous namespace

component create(const create_info& info, game_entity::entity entity)
{
    assert(entity.is_valid());

    if (const id::id_type ent_index = id::index(entity.get_id()); positions.size() > ent_index)
    {
        const vec4 rotation{ info.rotation };
        rotations[ent_index]                   = rotation;
        orientations[ent_index]                = calculate_orientation(rotation);
        positions[ent_index]                   = vec3{ info.position };
        scales[ent_index]                      = vec3{ info.scale };
        has_transform[ent_index]               = 0;
        changes_from_previous_frame[ent_index] = (u8) component_flags::all;
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
        changes_from_previous_frame.emplace_back((u8) component_flags::all);
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

void get_updated_components_flags(const game_entity::entity_id* const ids, u32 count, u8* const flags)
{
    assert(ids && count && flags);
    read_write_flag = 1;


    for (u32 i = 0; i < count; ++i)
    {
        assert(game_entity::entity{ ids[i] }.is_valid());
        flags[i] = changes_from_previous_frame[id::index(ids[i])];
    }
}

void update(const component_cache* const cache, u32 count)
{
    assert(cache && count);
    // NOTE: Clearing changes_from_previous_frame happens once each frame when there will
    // be no reads and the caches are about to be applied by calling this function -
    //  -- in other words, the rest of the frame will only have writes
    if (read_write_flag)
    {
        memset(changes_from_previous_frame.data(), 0, changes_from_previous_frame.size());
        read_write_flag = 0;
    }

    for (u32 i = 0; i < count; ++i)
    {
        const auto& [cached_rotation, cached_orientation, cached_position, cached_scale, cached_id, cached_flags]{ cache[i] };
        if (cached_flags & component_flags::rotation)
        {
            set_rotation(cached_id, cached_rotation);
        }
        if (cached_flags & component_flags::orientation)
        {
            set_orientation(cached_id, cached_orientation);
        }
        if (cached_flags & component_flags::position)
        {
            set_position(cached_id, cached_position);
        }
        if (cached_flags & component_flags::scale)
        {
            set_scale(cached_id, cached_scale);
        }
    }
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
