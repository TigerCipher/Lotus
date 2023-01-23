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
// File Name: D3D12GPass.cpp
// Date File Created: 01/22/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "D3D12GPass.h"
#include "D3D12Core.h"
#include "D3D12Shaders.h"

namespace lotus::graphics::d3d12::gpass
{

namespace
{

struct gpass_root_param_indices {
    enum : u32 {
        root_constants,


        count
    };
};

constexpr DXGI_FORMAT main_buffer_format{ DXGI_FORMAT_R16G16B16A16_FLOAT };
constexpr DXGI_FORMAT depth_buffer_format{ DXGI_FORMAT_D32_FLOAT };

constexpr vec2u initial_dimensions{ 100, 100 };

d3d12_render_texture gpass_main_buffer{};
d3d12_depth_buffer   gpass_depth_buffer{};

vec2u dimensions{ initial_dimensions };

ID3D12RootSignature* gpass_root_sig{ nullptr };
ID3D12PipelineState* gpass_pso{ nullptr };

#if L_DEBUG
constexpr f32 clear_value[4]{ 0.5f, 0.5f, 0.5f, 1.0f };
#else
constexpr f32 clear_value[4]{};
#endif

bool create_buffers(vec2u size)
{
    LASSERT(size.x && size.y);
    gpass_main_buffer.release();
    gpass_depth_buffer.release();

    D3D12_RESOURCE_DESC desc{};
    desc.Alignment        = 0;
    desc.DepthOrArraySize = 1;
    desc.Dimension        = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Flags            = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    desc.Width            = size.x;
    desc.Height           = size.y;
    desc.Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.MipLevels        = 0;
    desc.SampleDesc       = { 1, 0 };
    desc.Format           = main_buffer_format;

    // Main buffer
    {
        d3d12_texture_init_info info{};
        info.desc               = &desc;
        info.initial_state      = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        info.clear_value.Format = desc.Format;
        memcpy(&info.clear_value.Color, &clear_value[0], sizeof(clear_value));
        gpass_main_buffer = d3d12_render_texture{ info };
    }

    desc.Flags     = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    desc.Format    = depth_buffer_format;
    desc.MipLevels = 1;

    // Depth buffer
    {
        d3d12_texture_init_info info{};
        info.desc          = &desc;
        info.initial_state = D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                             D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        info.clear_value.Format               = desc.Format;
        info.clear_value.DepthStencil.Depth   = 0.0f;
        info.clear_value.DepthStencil.Stencil = 0;
        gpass_depth_buffer                    = d3d12_depth_buffer{ info };
    }

    NAME_D3D_OBJ(gpass_main_buffer.resource(), L"GPass Main Buffer");
    NAME_D3D_OBJ(gpass_depth_buffer.resource(), L"GPass Depth Buffer");

    return gpass_main_buffer.resource() && gpass_depth_buffer.resource();
}

bool create_gpass_pso_and_rootsig()
{
    LASSERT(!gpass_root_sig && !gpass_pso);

    using idx = gpass_root_param_indices;
    // root sig
    d3dx::d3d12_root_parameter params[idx::count]{};
    params[idx::root_constants].as_constants(3, D3D12_SHADER_VISIBILITY_PIXEL, 1);
    const d3dx::d3d12_root_signature_desc root_sig{ &params[0], _countof(params) };
    gpass_root_sig = root_sig.create();
    LASSERT(gpass_root_sig);

    NAME_D3D_OBJ(gpass_root_sig, L"GPass Root Signature");

    // pipeline state
    struct
    {
        d3dx::d3d12_pipeline_state_subobject_root_signature root_sig{ gpass_root_sig };

        d3dx::d3d12_pipeline_state_subobject_vs vs{ shaders::get_engine_shader(
            shaders::engine_shader::fullscreen_triangle_vs) };
        d3dx::d3d12_pipeline_state_subobject_ps ps{ shaders::get_engine_shader(shaders::engine_shader::fillcolor_ps) };
        d3dx::d3d12_pipeline_state_subobject_primitive_topology prim_topology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
        d3dx::d3d12_pipeline_state_subobject_render_target_formats render_target_formats;
        d3dx::d3d12_pipeline_state_subobject_depth_stencil_format  depth_stencil_formats;
        d3dx::d3d12_pipeline_state_subobject_rasterizer            rasterizer{ d3dx::rasterizer_state.no_cull };
        d3dx::d3d12_pipeline_state_subobject_depth_stencil1        depth{ d3dx::depth_state.disabled };
    } stream;

    D3D12_RT_FORMAT_ARRAY rtf_array{};
    rtf_array.NumRenderTargets = 1;
    rtf_array.RTFormats[0]     = main_buffer_format;

    stream.render_target_formats = rtf_array;
    gpass_pso                    = d3dx::create_pipeline_state(&stream, sizeof(stream));

    NAME_D3D_OBJ(gpass_pso, L"GPass Pipeline State Object");

    return gpass_root_sig && gpass_pso;
}

} // anonymous namespace

bool initialize()
{
    return create_buffers(initial_dimensions) && create_gpass_pso_and_rootsig();
}

void shutdown()
{
    gpass_main_buffer.release();
    gpass_depth_buffer.release();

    dimensions = initial_dimensions;

    core::release(gpass_root_sig);
    core::release(gpass_pso);
}

void set_size(vec2u size)
{
    vec2u& d = dimensions;

    if (size.x > d.x || size.y > d.y)
    {
        d = { std::max(size.x, d.x), std::max(size.y, d.y) };
        create_buffers(d);
    }
}

void depth_prepass(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& info) {}

void render(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& info)
{
    cmd_list->SetGraphicsRootSignature(gpass_root_sig);
    cmd_list->SetPipelineState(gpass_pso);


    static u32 frame = 0;
    struct {
        f32 width;
        f32 height;
        u32 frame;
    } constants{(f32)info.surface_width, (f32)info.surface_width, ++frame };

    using idx = gpass_root_param_indices;
    cmd_list->SetGraphicsRoot32BitConstants(idx::root_constants, 3, &constants, 0);

    cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmd_list->DrawInstanced(3, 1, 0, 0);
}


void set_render_targets_depth_prepass(id3d12_graphics_command_list* cmd_list)
{
    const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ gpass_depth_buffer.dsv() };
    cmd_list->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);
    cmd_list->OMSetRenderTargets(0, nullptr, 0, &dsv);
}

void set_render_targets_gpass(id3d12_graphics_command_list* cmd_list)
{
    const D3D12_CPU_DESCRIPTOR_HANDLE rtv{ gpass_main_buffer.rtv(0) };
    const D3D12_CPU_DESCRIPTOR_HANDLE dsv{ gpass_depth_buffer.dsv() };

    cmd_list->ClearRenderTargetView(rtv, clear_value, 0, nullptr);
    cmd_list->OMSetRenderTargets(1, &rtv, 0, &dsv);
}

void add_transitions_depth_prepass(d3dx::d3d12_resource_barrier& barriers)
{
    barriers.add(gpass_depth_buffer.resource(),
                 D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                 D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void add_transitions_gpass(d3dx::d3d12_resource_barrier& barriers)
{
    barriers.add(gpass_main_buffer.resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                 D3D12_RESOURCE_STATE_RENDER_TARGET);

    barriers.add(gpass_depth_buffer.resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
                 D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void add_transitions_post_process(d3dx::d3d12_resource_barrier& barriers)
{
    barriers.add(gpass_main_buffer.resource(), D3D12_RESOURCE_STATE_RENDER_TARGET,
                 D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

const d3d12_render_texture& main_buffer()
{
    return gpass_main_buffer;
}

const d3d12_depth_buffer& depth_buffer()
{
    return gpass_depth_buffer;
}

} // namespace lotus::graphics::d3d12::gpass