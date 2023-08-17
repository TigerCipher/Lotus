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
constexpr DXGI_FORMAT main_buffer_format{ DXGI_FORMAT_R16G16B16A16_FLOAT };
constexpr DXGI_FORMAT depth_buffer_format{ DXGI_FORMAT_D32_FLOAT };

struct opaque_root_parameter
{
    enum parameter : u32
    {
        global_shader_data,
        position_buffer,
        element_buffer,
        srv_indices,
        directional_lights,
        per_object_data,

        count
    };
};

bool initialize();
void shutdown();

void set_size(vec2u size);
void depth_prepass(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& d3d12_info);
void render(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& d3d12_info);

void add_transitions_depth_prepass(d3dx::d3d12_resource_barrier& barriers);
void add_transitions_gpass(d3dx::d3d12_resource_barrier& barriers);
void add_transitions_post_process(d3dx::d3d12_resource_barrier& barriers);

void set_render_targets_depth_prepass(id3d12_graphics_command_list* cmd_list);
void set_render_targets_gpass(id3d12_graphics_command_list* cmd_list);

const d3d12_render_texture& main_buffer();
const d3d12_depth_buffer&   depth_buffer();

} // namespace lotus::graphics::d3d12::gpass