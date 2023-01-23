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
utl::vector<vec3> positions;
utl::vector<vec4> rotations;
utl::vector<vec3> scales;
} // namespace

component create(const create_info& info, entity::entity entity)
{
    LASSERT(entity.is_valid());
    const id::id_type ent_index = id::index(entity.get_id());

    if (positions.size() > ent_index)
    {
        positions[ent_index] = vec3(info.position);
        rotations[ent_index] = vec4(info.rotation);
        scales[ent_index]    = vec3(info.scale);
    } else
    {
        LASSERT(positions.size() == ent_index);
        positions.emplace_back(info.position);
        rotations.emplace_back(info.rotation);
        scales.emplace_back(info.scale);
    }

    return component(transform_id{ entity.get_id() });
}

void remove([[maybe_unused]] const component comp)
{
    LASSERT(comp.is_valid());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TransformComponent Class Implementations ////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


vec3 component::position() const
{
    LASSERT(is_valid());
    return positions[id::index(m_id)];
}
vec4 component::rotation() const
{
    LASSERT(is_valid());
    return rotations[id::index(m_id)];
}
vec3 component::scale() const
{
    LASSERT(is_valid());
    return scales[id::index(m_id)];
}


} // namespace lotus::transform
