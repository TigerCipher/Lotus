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
#include "Entity.h"
#include "Transform.h"

#define USE_TRANSFORM_CACHE_MAP 0

namespace lotus::script
{


namespace
{
using script_registry = std::unordered_map<size_t, detail::script_creator>;

utl::vector<detail::script_ptr> entity_scripts;
utl::vector<id::id_type>        id_mapping;
utl::vector<id::gen_type>       generations;
utl::deque<script_id>           free_ids;

utl::vector<transform::component_cache> transform_cache;

#if USE_TRANSFORM_CACHE_MAP
std::unordered_map<id::id_type, u32> cache_map;
#endif

script_registry& registry()
{
    static script_registry reg;
    return reg;
}


bool exists(const script_id id)
{
    assert(id::is_valid(id));
    const id::id_type index = id::index(id);
    assert((index < generations.size() && id_mapping[index] < entity_scripts.size()) && generations[index] == id::generation(id));
    return (generations[index] == id::generation(id)) && entity_scripts[id_mapping[index]] &&
           entity_scripts[id_mapping[index]]->is_valid();
}

#ifdef L_EDITOR
utl::vector<std::string>& script_names()
{
    static utl::vector<std::string> names;
    return names;
}
#endif

#if USE_TRANSFORM_CACHE_MAP
transform::component_cache* const get_cache_ptr(const game_entity::entity* const entity)
{
    assert(game_entity::is_alive(entity->get_id()));
    const transform::transform_id id{ entity->transform().get_id() };

    u32 index{ invalid_id_u32 };
    const auto [fst, snd]{ cache_map.try_emplace(id, id::invalid_id) };

    if (snd)
    {
        index = (u32) transform_cache.size();
        transform_cache.emplace_back();
        transform_cache.back().id = id;
        cache_map[id]             = index;
    } else
    {
        index = cache_map[id];
    }

    assert(index < transform_cache.size());

    return &transform_cache[index];
}
#else
transform::component_cache* const get_cache_ptr(const game_entity::entity* const entity)
{
    assert(game_entity::is_alive(entity->get_id()));
    const transform::transform_id id{ entity->transform().get_id() };

    for (auto& cache : transform_cache)
    {
        if (cache.id == id)
        {
            return &cache;
        }
    }

    transform_cache.emplace_back();
    transform_cache.back().id = id;

    return &transform_cache.back();
}
#endif

} // anonymous namespace

namespace detail
{
u8 register_script(size_t tag, script_creator func)
{
    const bool res = registry().insert(script_registry::value_type{ tag, func }).second;
    assert(res);
    return res;
}

script_creator get_script_creator(const size_t tag)
{
    const auto script = registry().find(tag);
    assert(script != registry().end() && script->first == tag);
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


component create(const create_info& info, const game_entity::entity entity)
{
    assert(entity.is_valid() && info.script_creator);
    script_id id;

    if (free_ids.size() > id::min_deleted_elements)
    {
        id = free_ids.front();
        assert(!exists(id));
        free_ids.pop_front();
        id = script_id{ id::new_generation(id) };
        ++generations[id::index(id)];
    } else
    {
        id = script_id{ (id::id_type) id_mapping.size() };
        id_mapping.emplace_back();
        generations.push_back(0);
    }

    assert(id::is_valid(id));
    const auto index = (id::id_type) entity_scripts.size();
    entity_scripts.emplace_back(info.script_creator(entity));
    assert(entity_scripts.back()->get_id() == entity.get_id());

    id_mapping[id::index(id)] = index;
    return component(id);
}

void remove(const component comp)
{
    assert(comp.is_valid() && exists(comp.get_id()));
    const script_id   id      = comp.get_id();
    const id::id_type index   = id_mapping[id::index(id)];
    const script_id   last_id = entity_scripts.back()->script().get_id();
    utl::erase_unordered(entity_scripts, index);
    id_mapping[id::index(last_id)] = index;
    id_mapping[id::index(id)]      = id::invalid_id;
}

void update_all(f32 delta)
{
    for (const auto& scr : entity_scripts)
    {
        scr->update(delta);
    }

    if (!transform_cache.empty())
    {
        transform::update(transform_cache.data(), (u32) transform_cache.size());
        transform_cache.clear();
#if USE_TRANSFORM_CACHE_MAP
        cache_map.clear();
#endif
    }
}

// From API/GameEntity.h
void entity_script::set_rotation(const game_entity::entity* const entity, vec4 rotation_quaternion)
{
    transform::component_cache& cache{ *get_cache_ptr(entity) };
    cache.flags |= transform::component_flags::rotation;
    cache.rotation = rotation_quaternion;
}

void entity_script::set_orientation(const game_entity::entity* const entity, vec3 orientation_vector)
{
    transform::component_cache& cache{ *get_cache_ptr(entity) };
    cache.flags |= transform::component_flags::orientation;
    cache.orientation = orientation_vector;
}

void entity_script::set_position(const game_entity::entity* const entity, vec3 position)
{
    transform::component_cache& cache{ *get_cache_ptr(entity) };
    cache.flags |= transform::component_flags::position;
    cache.position = position;
}

void entity_script::set_scale(const game_entity::entity* const entity, vec3 scale)
{
    transform::component_cache& cache{ *get_cache_ptr(entity) };
    cache.flags |= transform::component_flags::scale;
    cache.scale = scale;
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