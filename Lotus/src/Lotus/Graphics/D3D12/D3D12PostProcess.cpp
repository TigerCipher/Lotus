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
// File Name: D3D12PostProcess.cpp
// Date File Created: 01/22/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "D3D12PostProcess.h"
#include "D3D12Shaders.h"
#include "D3D12Surface.h"
#include "D3D12GPass.h"
#include "D3D12Core.h"

namespace lotus::graphics::d3d12::fx
{

namespace
{

struct fx_root_param_indices {
    enum : u32 {
        root_constants,

        count
    };
};

ID3D12RootSignature* fx_root_sig{ nullptr };
ID3D12PipelineState* fx_pso{ nullptr };

bool create_fx_pso_root_sig()
{
    assert(!fx_root_sig && !fx_pso);

    // d3dx::d3d12_descriptor_range range{
    //     D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
    //     D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
    //     0,
    //     0,
    //     D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
    // };

    // fx sig
    using idx = fx_root_param_indices;
    d3dx::d3d12_root_parameter params[idx::count]{};
    params[idx::root_constants].as_constants(1, D3D12_SHADER_VISIBILITY_PIXEL, 1);
    // params[idx::descriptor_table].as_descriptor_table(D3D12_SHADER_VISIBILITY_PIXEL, &range, 1);

    d3dx::d3d12_root_signature_desc root_sig{ &params[0], idx::count };
    root_sig.Flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
    fx_root_sig = root_sig.create();
    assert(fx_root_sig);

    NAME_D3D_OBJ(fx_root_sig, L"Post-Process FX Root Signature");

    // pipeline state
    struct
    {
        d3dx::d3d12_pipeline_state_subobject_root_signature root_sig{ fx_root_sig };

        d3dx::d3d12_pipeline_state_subobject_vs                 vs{ shaders::get_engine_shader(
            shaders::engine_shader::fullscreen_triangle_vs) };
        d3dx::d3d12_pipeline_state_subobject_ps                 ps{ shaders::get_engine_shader(
            shaders::engine_shader::post_process_ps) };
        d3dx::d3d12_pipeline_state_subobject_primitive_topology prim_topology{ D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE };
        d3dx::d3d12_pipeline_state_subobject_render_target_formats render_target_formats;
        d3dx::d3d12_pipeline_state_subobject_rasterizer            rasterizer{ d3dx::rasterizer_state.no_cull };
    } stream;

    D3D12_RT_FORMAT_ARRAY rtf_array{};
    rtf_array.NumRenderTargets = 1;
    rtf_array.RTFormats[0]     = d3d12_surface::default_backbuffer_format;

    stream.render_target_formats = rtf_array;
    fx_pso                       = d3dx::create_pipeline_state(&stream, sizeof(stream));

    NAME_D3D_OBJ(fx_pso, L"Post-Process FX Pipeline State Object");

    return fx_root_sig && fx_pso;
}

} // anonymous namespace


bool initialize()
{
    return create_fx_pso_root_sig();
}

void shutdown()
{
    core::release(fx_root_sig);
    core::release(fx_pso);
}

void post_process(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& frame_info,
                  D3D12_CPU_DESCRIPTOR_HANDLE target_rtv)
{
    cmd_list->SetGraphicsRootSignature(fx_root_sig);
    cmd_list->SetPipelineState(fx_pso);

    using idx = fx_root_param_indices;
    cmd_list->SetGraphicsRoot32BitConstant(idx::root_constants, gpass::main_buffer().srv().index, 0);
    // cmd_list->SetGraphicsRootDescriptorTable(idx::descriptor_table, core::srv_heap().gpu_start());

    cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    cmd_list->OMSetRenderTargets(1, &target_rtv, 1, nullptr);
    cmd_list->DrawInstanced(3, 1, 0, 0);
}

} // namespace lotus::graphics::d3d12::fx
