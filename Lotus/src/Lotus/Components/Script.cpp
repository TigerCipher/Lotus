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
#include "Script.h"

namespace lotus::script
{


namespace
{
using script_registry = std::unordered_map<size_t, detail::script_creator>;

utl::vector<detail::script_ptr> entity_scripts;
utl::vector<id::id_type>        id_mapping;
utl::vector<id::gen_type>       generations;
utl::deque<script_id>           free_ids;

script_registry& registry()
{
    static script_registry reg;
    return reg;
}


bool exists(const script_id scriptId)
{
    LASSERT(id::is_valid(scriptId));
    const id::id_type index = id::index(scriptId);
    LASSERT((index < generations.size() && id_mapping[index] < entity_scripts.size()) &&
            generations[index] == id::generation(scriptId));
    return (generations[index] == id::generation(scriptId)) && entity_scripts[id_mapping[index]] &&
           entity_scripts[id_mapping[index]]->is_valid();
}

#ifdef L_EDITOR
utl::vector<std::string>& script_names()
{
    static utl::vector<std::string> names;
    return names;
}
#endif

} // anonymous namespace

namespace detail
{
byte register_script(size_t tag, script_creator func)
{
    const bool res = registry().insert(script_registry::value_type{ tag, func }).second;
    LASSERT(res);
    return res;
}

script_creator get_script_creator(const size_t tag)
{
    const auto script = registry().find(tag);
    LASSERT(script != registry().end() && script->first == tag);
    return script->second;
}


#ifdef L_EDITOR
u8 add_script_name(const char* name)
{
    script_names().emplace_back(name);
    return true;
}
#endif

} // namespace detail


component create(const create_info& info, const entity::entity entity)
{
    LASSERT(entity.is_valid() && info.script_creator);
    script_id scriptId;

    if (free_ids.size() > id::min_deleted_elements)
    {
        scriptId = free_ids.front();
        LASSERT(!exists(scriptId));
        free_ids.pop_front();
        scriptId = script_id{ id::new_generation(scriptId) };
        ++generations[id::index(scriptId)];
    } else
    {
        scriptId = script_id{ (id::id_type) id_mapping.size() };
        id_mapping.emplace_back();
        generations.push_back(0);
    }

    LASSERT(id::is_valid(scriptId));
    const id::id_type index = (id::id_type) entity_scripts.size();
    entity_scripts.emplace_back(info.script_creator(entity));
    LASSERT(entity_scripts.back()->get_id() == entity.get_id());

    id_mapping[id::index(scriptId)] = index;
    return component(scriptId);
}

void remove(const component comp)
{
    assert(comp.is_valid() && exists(comp.get_id()));
    const script_id   id     = comp.get_id();
    const id::id_type index  = id_mapping[id::index(id)];
    const script_id   lastId = entity_scripts.back()->script().get_id();
    utl::erase_unordered(entity_scripts, index);
    id_mapping[id::index(lastId)] = index;
    id_mapping[id::index(id)]     = id::invalid_id;
}

void update_all(timestep ts)
{
    for (const auto& scr : entity_scripts)
    {
        scr->update(ts);
    }
}
} // namespace lotus::script

#ifdef L_EDITOR

    #include <atlsafe.h>

extern "C" __declspec(dllexport) LPSAFEARRAY get_script_names()
{
    const u32 size = (u32) lotus::script::script_names().size();
    if (!size)
        return nullptr;
    CComSafeArray<BSTR> names(size);
    for (u32 i = 0; i < size; ++i)
    {
        names.SetAt(i, A2BSTR_EX(lotus::script::script_names()[i].c_str()), false);
    }

    return names.Detach();
}

#endif