﻿// ------------------------------------------------------------------------------
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
// File Name: D3D12Core.h
// Date File Created: 10/30/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "D3D12Common.h"

namespace lotus::graphics::d3d12
{

namespace camera
{
class d3d12_camera;
}

struct d3d12_frame_info
{
    const frame_info*         info{};
    camera::d3d12_camera*     camera{};
    D3D12_GPU_VIRTUAL_ADDRESS global_shader_data{};
    u32                       surface_width{};
    u32                       surface_height{};
    u32                       frame_index{};
    f32                       delta_time{};
};
} // namespace lotus::graphics::d3d12

namespace lotus::graphics::d3d12::core
{
bool initialize();
void shutdown();

template<typename T>
constexpr void release(T*& resource)
{
    if (resource)
    {
        resource->Release();
        resource = nullptr;
    }
}

namespace detail
{
void deferred_release(IUnknown* resource);
}

template<typename T>
constexpr void deferred_release(T*& resource)
{
    if (resource)
    {
        detail::deferred_release(resource);
        resource = nullptr;
    }
}

[[nodiscard]] id3d12_device* const device();

u32 current_frame_index();

void set_derferred_releases_flag();

[[nodiscard]] descriptor_heap& rtv_heap();
[[nodiscard]] descriptor_heap& dsv_heap();
[[nodiscard]] descriptor_heap& srv_heap();
[[nodiscard]] descriptor_heap& uav_heap();
[[nodiscard]] constant_buffer& cbuffer();


[[nodiscard]] surface create_surface(platform::window window);
void                  remove_surface(surface_id id);
void                  resize_surface(surface_id id, u32 width, u32 height);
[[nodiscard]] u32     surface_width(surface_id id);
[[nodiscard]] u32     surface_height(surface_id id);
void                  render_surface(surface_id id, frame_info info);

} // namespace lotus::graphics::d3d12::core