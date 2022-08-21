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

namespace lotus::ecs
{

namespace
{
    utl::vector<vec3> positions;
    utl::vector<vec4> rotations;
    utl::vector<vec3> scales;
} // namespace

TransformComponent CreateTransform(const TransformData& desc, Entity entity)
{
    LASSERT(entity.IsValid());
    const id::id_type entIndex = id::Index(entity.GetId());

    if (positions.size() > entIndex)
    {
        positions [ entIndex ] = vec3(desc.position);
        rotations [ entIndex ] = vec4(desc.rotation);
        scales [ entIndex ]    = vec3(desc.scale);
    } else
    {
        LASSERT(positions.size() == entIndex);
        positions.emplace_back(desc.position);
        rotations.emplace_back(desc.rotation);
        scales.emplace_back(desc.scale);
    }

    return TransformComponent(TransformId { (id::id_type) positions.size() - 1 });
}

void RemoveTransform(const TransformComponent comp) { LASSERT(comp.IsValid()); }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TransformComponent Class Implementations ////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



vec3 TransformComponent::Position() const
{
    LASSERT(IsValid());
    return positions [ id::Index(mId) ];
}
vec4 TransformComponent::Rotation() const
{
    LASSERT(IsValid());
    return rotations [ id::Index(mId) ];
}
vec3 TransformComponent::Scale() const
{
    LASSERT(IsValid());
    return scales [ id::Index(mId) ];
}


} // namespace lotus::ecs
