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
// File Name: Entity.h
// Date File Created: 08/19/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "Components.h"

#define COMPONENT_DESCRIPTOR(name) struct name##Desc;


namespace lotus
{
COMPONENT_DESCRIPTOR(Transform)
// more components
#undef COMPONENT_DESCRIPTOR

struct EntityInfo
{
    TransformDesc* transform = nullptr;
};

EntityId CreateGameEntity(const EntityInfo& info);
void     RemoveEntity(EntityId id);
bool     IsAlive(EntityId id);


} // namespace lotus
