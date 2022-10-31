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
// File Name: Renderer.h
// Date File Created: 08/29/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "Lotus/Core/Common.h"
#include "Lotus/Platform/Window.h"


namespace lotus::graphics
{
class surface
{};

struct render_surface
{
    platform::window window{};
    surface          surface{};
};

enum class graphics_platform : u32
{
    d3d12 = 0,
    //TODO: vulkan = 1
};

bool initialize(graphics_platform platform);

void shutdown();

void render();

} // namespace lotus::graphics
