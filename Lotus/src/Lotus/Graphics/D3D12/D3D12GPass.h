// ------------------------------------------------------------------------------
//
// Lotus
//    Copyright 2023 Matthew Rogers
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
// File Name: D3D12GPass
// Date File Created: 1/22/2023 12:56:14 PM
// Author: Matt
//
// ------------------------------------------------------------------------------

#pragma once


#include "D3D12Common.h"

namespace lotus::graphics::d3d12
{
struct d3d12_frame_info;
}

namespace lotus::graphics::d3d12::gpass
{

bool initialize();
void shutdown();

void set_size(vec2u size);
void depth_prepass(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& info);
void render(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& info);

}