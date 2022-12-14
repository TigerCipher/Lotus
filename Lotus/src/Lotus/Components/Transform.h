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
#include "Components.h"


namespace lotus::transform
{

struct create_info
{
    f32 position [ 3 ] {};
    f32 rotation [ 4 ] {};
    f32 scale [ 3 ] { 1.0f, 1.0f, 1.0f };
};

Component create(const create_info& info, entity::entity entity);
void      remove(Component comp);

} // namespace lotus::transform
