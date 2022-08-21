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
// File Name: Transform.h
// Date File Created: 8/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once
#include "Lotus/EngineApi/GameEntity.h"
#include "Lotus/EngineApi/TransformComponent.h"


namespace lotus
{

// #CONSIDER Using other "descriptors" for the editor interface... maybe I should follow a lotus::ecs::transform, lotus::ecs::entity type of namespace system?
struct TransformInfo
{
    f32 position [ 3 ] {};
    f32 rotation [ 4 ] {};
    f32 scale [ 3 ] { 1.0f, 1.0f, 1.0f };
};

TransformComponent CreateTransform(const TransformInfo& desc, Entity entity);
void      RemoveTransform(TransformComponent comp);

} // namespace lotus::ecs
