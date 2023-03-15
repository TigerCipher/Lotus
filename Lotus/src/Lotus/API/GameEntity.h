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
// File Name: GameEntity.h
// Date File Created: 08/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once
#include "../Components/Components.h"
#include "../Core/Timestep.h"
#include "TransformComponent.h"
#include "ScriptComponent.h"

namespace lotus
{
namespace game_entity
{
L_TYPED_ID(entity_id)

class entity
{
public:
    constexpr explicit entity(const entity_id id) : m_id(id) {}
    constexpr entity() : m_id(id::invalid_id) {}

    constexpr entity_id get_id() const { return m_id; }

    constexpr bool is_valid() const { return id::is_valid(m_id); }

    transform::component transform() const;
    script::component    script() const;

private:
    entity_id m_id;
};
} // namespace game_entity

namespace script
{
class scriptable_entity : public game_entity::entity
{
public:
    virtual ~scriptable_entity() = default;

    virtual void on_start() {}
    virtual void update([[maybe_unused]] timestep ts) {}

protected:
    constexpr explicit scriptable_entity(const entity entity) : entity(entity.get_id()) {}
};


namespace detail
{
using script_ptr     = scope<scriptable_entity>;
using script_creator = script_ptr (*)(game_entity::entity entity);

u8 register_script(size_t tag, script_creator func);

L_EXPORT script_creator get_script_creator(size_t tag);

template<class T>
script_ptr create_script(game_entity::entity entity)
{
    assert(entity.is_valid());
    return create_scope<T>(entity);
}

#ifdef L_EDITOR
u8 add_script_name(const char* name);

    #define LOTUS_REGISTER_SCRIPT(Type)                                                                                     \
        namespace                                                                                                           \
        {                                                                                                                   \
        const u8 reg_##Type =                                                                                               \
            lotus::script::detail::register_script(string_hash()(#Type), &lotus::script::detail::create_script<Type>);      \
        const uint8 name_##Type = lotus::script::detail::add_script_name(#Type);                                            \
        }
#else

    #define LOTUS_REGISTER_SCRIPT(Type)                                                                                     \
        namespace                                                                                                           \
        {                                                                                                                   \
        const u8 reg_##Type =                                                                                               \
            lotus::script::detail::register_script(string_hash()(#Type), &lotus::script::detail::create_script<Type>);      \
        }
#endif
} // namespace detail
} // namespace script


} // namespace lotus
