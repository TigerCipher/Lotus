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
#include "pch.h"
#include "Transform.h"

namespace lotus::transform
{

namespace
{
    utl::vector<vec3> positions;
    utl::vector<vec4> rotations;
    utl::vector<vec3> scales;
} // namespace

Component create(const create_info& info, entity::Entity entity)
{
    LASSERT(entity.IsValid());
    const id::id_type entIndex = id::index(entity.GetId());

    if (positions.size() > entIndex)
    {
        positions [ entIndex ] = vec3(info.position);
        rotations [ entIndex ] = vec4(info.rotation);
        scales [ entIndex ]    = vec3(info.scale);
    } else
    {
        LASSERT(positions.size() == entIndex);
        positions.emplace_back(info.position);
        rotations.emplace_back(info.rotation);
        scales.emplace_back(info.scale);
    }

    return Component(transform_id { (id::id_type) positions.size() - 1 });
}

void remove(const Component comp) { LASSERT(comp.IsValid()); }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TransformComponent Class Implementations ////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


vec3 Component::Position() const
{
    LASSERT(IsValid());
    return positions [ id::index(mId) ];
}
vec4 Component::Rotation() const
{
    LASSERT(IsValid());
    return rotations [ id::index(mId) ];
}
vec3 Component::Scale() const
{
    LASSERT(IsValid());
    return scales [ id::index(mId) ];
}


} // namespace lotus::transform
