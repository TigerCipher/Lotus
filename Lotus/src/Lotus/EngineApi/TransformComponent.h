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
// File Name: TransformComponent.h
// Date File Created: 8/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once
#include "Lotus/Core/Id.h"
#include "Lotus/Core/Common.h"

namespace lotus
{
L_TYPED_ID(TransformId)


class TransformComponent final
{
public:
    constexpr explicit TransformComponent(const TransformId id) : mId(id) { }
    constexpr TransformComponent() : mId(id::InvalidId) { }

    constexpr TransformId GetId() const { return mId; }

    constexpr bool IsValid() const { return id::IsValid(mId); }

    vec3 Position() const;
    vec4 Rotation() const;
    vec3 Scale() const;

private:
    TransformId mId;
};
} // namespace lotus::ecs
