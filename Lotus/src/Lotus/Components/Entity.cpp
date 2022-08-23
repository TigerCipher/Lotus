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
#include "Script.h"


namespace lotus::entity
{
namespace
{
    utl::vector<id::gen_type>         generations;
    utl::deque<entity_id>             freeIds;
    utl::vector<transform::Component> transforms;
    utl::vector<script::Component>    scripts;
} // namespace

Entity CreateEntity(const create_info& info)
{
    LASSERT(info.transform);
    if (!info.transform) return {};

    entity_id ident;
    if (freeIds.size() > id::MinDeletedElements)
    {
        ident = freeIds.front();
        LASSERT(!IsAlive(ident));
        freeIds.pop_front();
        ident = entity_id { id::new_generation(ident) };
        ++generations [ id::index(ident) ];
    } else
    {
        ident = entity_id { (id::id_type) generations.size() };
        generations.push_back(0);

        // transforms.resize(generations.size()) // --> more memory allocations than emplace_back
        transforms.emplace_back();
    }

    const Entity      newEnt(ident);
    const id::id_type index = id::index(ident);

    LASSERT(!transforms [ index ].IsValid());
    transforms [ index ] = transform::create(*info.transform, newEnt);
    if (!transforms [ index ].IsValid()) return {};

    // Script
    if (info.script && info.script->scriptCreator)
    {
        LASSERT(!scripts [ index ].IsValid());
        scripts [ index ] = script::create(*info.script, newEnt);
        LASSERT(scripts [ index ].IsValid());
    }

    return newEnt;
}

void RemoveEntity(const entity_id id)
{
    const id::id_type index = id::index(id);
    LASSERT(IsAlive(id));
    transform::remove(transforms [ index ]);
    transforms [ index ] = {};
    freeIds.push_back(id);
}

bool IsAlive(const entity_id id)
{
    LASSERT(id::is_valid(id));
    const id::id_type index = id::index(id);
    LASSERT(index < generations.size());
    LASSERT(generations [ index ] == id::generation(id));
    return generations [ index ] == id::generation(id) && transforms [ index ].IsValid();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entity Class Implementations ////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

transform::Component Entity::Transform() const
{
    LASSERT(IsAlive(mId));
    const id::id_type index = id::index(mId);
    return transforms [ index ];
}


script::Component Entity::Script() const
{
    LASSERT(IsAlive(mId));
    const id::id_type index = id::index(mId);
    return scripts [ index ];
}


} // namespace lotus::entity
