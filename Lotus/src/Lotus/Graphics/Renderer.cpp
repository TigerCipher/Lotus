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
// File Name: Renderer.cpp
// Date File Created: 10/30/2022
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "Renderer.h"

#include "GraphicsPlatformInterface.h"
#include "D3D12/D3D12Interface.h"

namespace lotus::graphics
{
namespace
{
platform_interface gfx{};

bool set_platform_interface(graphics_platform platform)
{
    switch(platform)
    {
    case graphics_platform::d3d12:
        d3d12::get_platform_interface(gfx);
        break;
    default:
        return false;
    }

    return true;
}
} // namespace

bool initialize(graphics_platform platform)
{
    return set_platform_interface(platform) && gfx.initialize();
}

void shutdown()
{
    gfx.shutdown();
}

} // namespace lotus::graphics