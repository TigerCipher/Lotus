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

#include "D3D12Core.h"
#include "D3D12Content.h"
#include "D3D12Camera.h"

namespace lotus::graphics::d3d12
{
void get_platform_interface(platform_interface& pinterface)
{
    pinterface.initialize = core::initialize;
    pinterface.shutdown   = core::shutdown;

    pinterface.surface.create = core::create_surface;
    pinterface.surface.remove = core::remove_surface;
    pinterface.surface.resize = core::resize_surface;
    pinterface.surface.width  = core::surface_width;
    pinterface.surface.height = core::surface_height;
    pinterface.surface.render = core::render_surface;

    pinterface.camera.create        = camera::create;
    pinterface.camera.remove        = camera::remove;
    pinterface.camera.set_parameter = camera::set_parameter;
    pinterface.camera.get_parameter = camera::get_parameter;

    pinterface.resources.add_submesh        = content::submesh::add;
    pinterface.resources.remove_submesh     = content::submesh::remove;
    pinterface.resources.add_material       = content::material::add;
    pinterface.resources.remove_material    = content::material::remove;
    pinterface.resources.add_render_item    = content::render_item::add;
    pinterface.resources.remove_render_item = content::render_item::remove;


    pinterface.platform = graphics_platform::d3d12;
}
} // namespace lotus::graphics::d3d12