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
namespace entity
{
    L_TYPED_ID(entity_id)

    class Entity
    {
    public:
        constexpr explicit Entity(const entity_id id) : mId(id) { }
        constexpr Entity() : mId(id::InvalidId) { }

        constexpr entity_id GetId() const { return mId; }

        constexpr bool IsValid() const { return id::is_valid(mId); }

        transform::Component Transform() const;
        script::Component    Script() const;

    private:
        entity_id mId;
    };
} // namespace entity

namespace script
{
    class ScriptableEntity : public entity::Entity
    {
    public:
        virtual ~ScriptableEntity() = default;

        virtual void OnStart() { }
        virtual void Update(Timestep ts) { }

    protected:
        constexpr explicit ScriptableEntity(const Entity entity) : Entity(entity.GetId()) { }
    };

    namespace detail
    {
        using script_ptr     = Scope<ScriptableEntity>;
        using script_creator = script_ptr (*)(entity::Entity entity);

        byte register_script(size_t tag, script_creator func);

        template<class T>
        script_ptr create_script(entity::Entity entity)
        {
            LASSERT(entity.IsValid());
            return CreateScope<T>(entity);
        }

#define LOTUS_REGISTER_SCRIPT(Type)                                                                                    \
    class Type; /* NOLINT(bugprone-macro-parentheses) */                                                               \
    namespace                                                                                                          \
    {                                                                                                                  \
        const uint8 reg_##Type =                                                                                       \
            lotus::script::detail::register_script(string_hash()(#Type), &lotus::script::detail::create_script<Type>); \
    }
    } // namespace detail
} // namespace script


} // namespace lotus
