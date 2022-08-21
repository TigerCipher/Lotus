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
// File Name: Entity.cpp
// Date File Created: 8/19/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "pch.h"
#include "Entity.h"
#include "Transform.h"


namespace lotus
{
namespace
{
    utl::vector<id::gen_type>       generations;
    utl::deque<EntityId>            freeIds;
    utl::vector<TransformComponent> transforms;
} // namespace

Entity CreateEntity(const EntityInfo& desc)
{
    LASSERT(desc.transform);
    if (!desc.transform) return {};

    EntityId ident;
    if (freeIds.size() > id::MinDeletedElements)
    {
        ident = freeIds.front();
        LASSERT(!IsAlive(Entity(ident)));
        freeIds.pop_front();
        ident = EntityId { id::NewGeneration(ident) };
        ++generations [ id::Index(ident) ];
    } else
    {
        ident = EntityId { (id::id_type) generations.size() };
        generations.push_back(0);

        // transforms.resize(generations.size()) // --> more memory allocations than emplace_back
        transforms.emplace_back();
    }

    const Entity      newEnt(ident);
    const id::id_type index = id::Index(ident);

    LASSERT(!transforms [ index ].IsValid());
    transforms [ index ] = CreateTransform(*desc.transform, newEnt);
    if (!transforms [ index ].IsValid()) return {};

    return newEnt;
}

void RemoveEntity(const Entity ent)
{
    const EntityId    ident = ent.GetId();
    const id::id_type index = id::Index(ident);
    LASSERT(IsAlive(ent));
    RemoveTransform(transforms [ index ]);
    transforms [ index ] = {};
    freeIds.push_back(ident);
}

bool IsAlive(const Entity ent)
{
    LASSERT(ent.IsValid());
    const EntityId    ident = ent.GetId();
    const id::id_type index = id::Index(ident);
    LASSERT(index < generations.size());
    LASSERT(generations [ index ] == id::Generation(ident));
    return generations [ index ] == id::Generation(ident) && transforms [ index ].IsValid();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entity Class Implementations ////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TransformComponent Entity::Transform() const
{
    LASSERT(IsAlive(*this));
    const id::id_type index = id::Index(mId);
    return transforms [ index ];
}


} // namespace lotus
