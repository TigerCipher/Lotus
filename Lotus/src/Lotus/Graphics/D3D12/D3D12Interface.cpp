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
// File Name: D3D12Interface.cpp
// Date File Created: 10/30/2022
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "D3D12Interface.h"

#include "Lotus/Core/Common.h"
#include "Lotus/Graphics/Renderer.h"
#include "D3D12Core.h"

namespace lotus::graphics::d3d12
{
void get_platform_interface(platform_interface& pinterface)
{
    pinterface.initialize = core::initialize;
    pinterface.shutdown = core::shutdown;
    pinterface.render = core::render;
}
}
