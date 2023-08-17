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
// File Name: D3D12Light
// Date File Created: 8/16/2023 3:57:35 PM
// Author: Matt
//
// ------------------------------------------------------------------------------

#pragma once


#include "D3D12Common.h"

namespace lotus::graphics::d3d12
{
struct d3d12_frame_info;
}
namespace lotus::graphics::d3d12::light
{

bool initialize();
void shutdown();

graphics::light create(light_init_info info);
void            remove(light_id id, u64 light_set_key);
void set_parameter(light_id id, u64 light_set_key, light_parameter::parameter param, const void* const data, u32 data_size);
void get_parameter(light_id id, u64 light_set_key, light_parameter::parameter param, void* const data, u32 data_size);


void                      update_light_buffers(const d3d12_frame_info& d3d12_info);
D3D12_GPU_VIRTUAL_ADDRESS non_cullable_light_buffer(u32 frame_index);
u32                       non_cullable_light_count(u64 light_set_key);

} // namespace lotus::graphics::d3d12::light