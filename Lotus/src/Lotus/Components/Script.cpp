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
// File Name: Script.cpp
// Date File Created: 8/23/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "pch.h"
#include "Script.h"

namespace lotus::script
{


namespace
{
    using script_registry = std::unordered_map<size_t, detail::script_creator>;

    utl::vector<detail::script_ptr> entityScripts;
    utl::vector<id::id_type>        idMapping;
    utl::vector<id::gen_type>       generations;
    utl::vector<script_id>          freeIds;

    script_registry& registry()
    {
        static script_registry reg;
        return reg;
    }


    bool exists(const script_id scriptId)
    {
        LASSERT(id::is_valid(scriptId));
        const id::id_type index = id::index(scriptId);
        LASSERT((index < generations.size() && idMapping [ index ] < entityScripts.size()) &&
                generations [ index ] == id::generation(scriptId));
        return (generations [ index ] == id::generation(scriptId)) && entityScripts [ idMapping [ index ] ] &&
               entityScripts [ idMapping [ index ] ]->IsValid();
    }

} // namespace

namespace detail
{
    byte register_script(size_t tag, script_creator func)
    {
        bool res = registry().insert(script_registry::value_type { tag, func }).second;
        LASSERT(res);
        return res;
    }
} // namespace detail


Component create(const create_info& info, entity::Entity entity)
{
    LASSERT(entity.IsValid() && info.scriptCreator);
    script_id scriptId;

    if (freeIds.size() > id::MinDeletedElements)
    {
        scriptId = freeIds.front();
        LASSERT(!exists(scriptId));
        freeIds.pop_back();
        scriptId = script_id { id::new_generation(scriptId) };
        ++generations [ id::index(scriptId) ];
    } else
    {
        scriptId = script_id { (id::id_type) idMapping.size() };
        idMapping.emplace_back();
        generations.push_back(0);
    }

    LASSERT(id::is_valid(scriptId));
    const id::id_type index = (id::id_type) entityScripts.size();
    entityScripts.emplace_back(info.scriptCreator(entity));
    LASSERT(entityScripts.back()->GetId() == entity.GetId());

    idMapping [ id::index(scriptId) ] = index;
    return Component(scriptId);
}

void remove(Component comp)
{
    assert(comp.IsValid() && exists(comp.GetId()));
    const script_id   id     = comp.GetId();
    const id::id_type index  = idMapping [ id::index(id) ];
    const script_id   lastId = entityScripts.back()->Script().GetId();
    utl::erase_unordered(entityScripts, index);
    idMapping [ id::index(lastId) ] = index;
    idMapping [ id::index(id) ]     = id::InvalidId;
}
} // namespace lotus::script