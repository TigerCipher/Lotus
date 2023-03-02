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
// File Name: Script.h
// Date File Created: 8/23/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once
#include "Components.h"

namespace lotus::script
{

struct create_info
{
    detail::script_creator script_creator{};
};

component create(const create_info& info, game_entity::entity entity);
void      remove(component comp);
void      update_all(timestep ts);

} // namespace lotus::script
