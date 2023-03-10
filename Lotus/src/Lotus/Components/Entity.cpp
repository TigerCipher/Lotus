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
#include "Entity.h"
#include "Transform.h"
#include "Script.h"


namespace lotus::game_entity
{
namespace
{
utl::vector<id::gen_type>         generations;
utl::deque<entity_id>             free_ids;
utl::vector<transform::component> transforms;
utl::vector<script::component>    scripts;
} // anonymous namespace

entity create(const create_info& info)
{
    LASSERT(info.transform);
    if (!info.transform)
        return {};

    entity_id ident;
    if (free_ids.size() > id::min_deleted_elements)
    {
        ident = free_ids.front();
        LASSERT(!is_alive(ident));
        free_ids.pop_front();
        ident = entity_id{ id::new_generation(ident) };
        ++generations[id::index(ident)];
    } else
    {
        ident = entity_id{ (id::id_type) generations.size() };
        generations.push_back(0);

        // transforms.resize(generations.size()) // --> more memory allocations than emplace_back
        transforms.emplace_back();
        scripts.emplace_back();
    }

    const entity      new_ent(ident);
    const id::id_type index = id::index(ident);

    LASSERT(!transforms[index].is_valid());
    transforms[index] = transform::create(*info.transform, new_ent);
    if (!transforms[index].is_valid())
        return {};

    // Script
    if (info.script && info.script->script_creator)
    {
        LASSERT(!scripts[index].is_valid());
        scripts[index] = script::create(*info.script, new_ent);
        LASSERT(scripts[index].is_valid());
    }

    return new_ent;
}

void remove(const entity_id id)
{
    const id::id_type index = id::index(id);
    LASSERT(is_alive(id));

    if (scripts[index].is_valid())
    {
        script::remove(scripts[index]);
        scripts[index] = {};
    }

    transform::remove(transforms[index]);
    transforms[index] = {};
    free_ids.push_back(id);
}

bool is_alive(const entity_id id)
{
    LASSERT(id::is_valid(id));
    const id::id_type index = id::index(id);
    LASSERT(index < generations.size());
    return generations[index] == id::generation(id) && transforms[index].is_valid();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entity Class Implementations ////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

transform::component entity::transform() const
{
    LASSERT(is_alive(m_id));
    const id::id_type index = id::index(m_id);
    return transforms[index];
}


script::component entity::script() const
{
    LASSERT(is_alive(m_id));
    const id::id_type index = id::index(m_id);
    return scripts[index];
}


} // namespace lotus::game_entity
