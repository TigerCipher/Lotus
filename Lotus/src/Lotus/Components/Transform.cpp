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
utl::vector<vec4> rotations;
utl::vector<vec3> orientations;
utl::vector<vec3> positions;
utl::vector<vec3> scales;

vec3 calculate_orientation(vec4 rotation)
{
    const vec rotation_quat = math::load_float4(&rotation);
    const vec front         = math::set_vector(0.0f, 0.0f, 1.0f, 0.0f);
    vec3      orientation;
    math::store_float3(&orientation, math::rotate_vec3(front, rotation_quat));

    return orientation;
}

} // anonymous namespace

component create(const create_info& info, game_entity::entity entity)
{
    assert(entity.is_valid());

    if (const id::id_type ent_index = id::index(entity.get_id()); positions.size() > ent_index)
    {
        const vec4 rotation {info.rotation};
        rotations[ent_index] = rotation;
        orientations[ent_index] = calculate_orientation(rotation);
        positions[ent_index] = vec3{info.position};
        scales[ent_index]    = vec3{info.scale};
    } else
    {
        assert(positions.size() == ent_index);
        rotations.emplace_back(info.rotation);
        orientations.emplace_back(calculate_orientation(vec4{info.rotation}));
        positions.emplace_back(info.position);
        scales.emplace_back(info.scale);
    }

    return component(transform_id{ entity.get_id() });
}

void remove([[maybe_unused]] const component comp)
{
    assert(comp.is_valid());
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
