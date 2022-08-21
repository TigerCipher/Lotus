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
#include "Lotus/Core/Common.h"
#include "TransformComponent.h"

namespace lotus
{

L_TYPED_ID(EntityId)

class Entity
{
public:
    constexpr explicit Entity(const EntityId id) : mId(id) { }
    constexpr Entity() : mId(id::InvalidId) { }

    constexpr EntityId GetId() const { return mId; }

    constexpr bool IsValid() const { return id::IsValid(mId); }

    TransformComponent Transform() const;

private:
    EntityId mId;
};
} // namespace lotus::ecs
