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
// File Name: ContentLoader.h
// Date File Created: 08/26/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "../Core/Common.h"

#ifndef PRODUCTION
namespace lotus::content
{
bool load_game();
void unload_game();
bool load_engine_shaders(scope<u8[]>& shaders_blob, u64& size);
} // namespace lotus::content
#endif