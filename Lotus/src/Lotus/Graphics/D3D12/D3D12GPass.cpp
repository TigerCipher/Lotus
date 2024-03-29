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
#include "D3D12Camera.h"
#include "D3D12Content.h"
#include "D3D12Light.h"
#include "Shaders/SharedTypes.h"
#include "Components/Transform.h"

namespace lotus::graphics::d3d12::gpass
{

namespace
{


constexpr vec2u initial_dimensions{ 100, 100 };

d3d12_render_texture gpass_main_buffer{};
d3d12_depth_buffer   gpass_depth_buffer{};

vec2u dimensions{ initial_dimensions };


#if L_DEBUG
//constexpr f32 clear_value[4]{ 0.5f, 0.0f, 0.5f, 1.0f };
constexpr f32 clear_value[4]{ 0.5f, 0.5f, 0.5f, 1.0f };
#else
constexpr f32 clear_value[4]{};
#endif

#if USE_STL_VECTOR
    #define CONSTEXPR
#else
    #define CONSTEXPR constexpr
#endif

struct gpass_cache
{
    utl::vector<id::id_type>   d3d12_render_item_ids;
    id::id_type*               entity_ids{ nullptr };
    id::id_type*               submesh_gpu_ids{ nullptr };
    id::id_type*               material_ids{ nullptr };
    ID3D12PipelineState**      gpass_pipeline_states{ nullptr };
    ID3D12PipelineState**      depth_pipeline_states{ nullptr };
    ID3D12RootSignature**      root_signatures{ nullptr };
    material_type::type*       material_types{ nullptr };
    D3D12_GPU_VIRTUAL_ADDRESS* position_buffers{ nullptr };
    D3D12_GPU_VIRTUAL_ADDRESS* element_buffers{ nullptr };
    D3D12_INDEX_BUFFER_VIEW*   index_buffer_views{ nullptr };
    D3D_PRIMITIVE_TOPOLOGY*    primitive_topologies{ nullptr };
    u32*                       elements_types{ nullptr };
    D3D12_GPU_VIRTUAL_ADDRESS* per_object_data{ nullptr };

    [[nodiscard]] constexpr content::render_item::items_cache items_cache() const
    {
        return { entity_ids, submesh_gpu_ids, material_ids, gpass_pipeline_states, depth_pipeline_states };
    }

    [[nodiscard]] constexpr content::submesh::views_cache views_cache() const
    {
        return { position_buffers, element_buffers, index_buffer_views, primitive_topologies, elements_types };
    }

    [[nodiscard]] constexpr content::material::materials_cache materials_cache() const
    {
        return { root_signatures, material_types };
    }

    [[nodiscard]] CONSTEXPR u32 size() const { return (u32) d3d12_render_item_ids.size(); }

    CONSTEXPR void clear() { d3d12_render_item_ids.clear(); }

    CONSTEXPR void resize()
    {
        const u64 items_count{ d3d12_render_item_ids.size() };
        const u64 new_buffer_size{ items_count * struct_size };
        const u64 old_buffer_size{ m_buffer.size() };
        if (new_buffer_size > old_buffer_size)
        {
            m_buffer.resize(new_buffer_size);
        }

        if (new_buffer_size != old_buffer_size)
        {
            entity_ids            = (id::id_type*) m_buffer.data();
            submesh_gpu_ids       = (id::id_type*) &entity_ids[items_count];
            material_ids          = (id::id_type*) &submesh_gpu_ids[items_count];
            gpass_pipeline_states = (ID3D12PipelineState**) &material_ids[items_count];
            depth_pipeline_states = (ID3D12PipelineState**) &gpass_pipeline_states[items_count];
            root_signatures       = (ID3D12RootSignature**) &depth_pipeline_states[items_count];
            material_types        = (material_type::type*) &root_signatures[items_count];
            position_buffers      = (D3D12_GPU_VIRTUAL_ADDRESS*) &material_types[items_count];
            element_buffers       = (D3D12_GPU_VIRTUAL_ADDRESS*) &position_buffers[items_count];
            index_buffer_views    = (D3D12_INDEX_BUFFER_VIEW*) &element_buffers[items_count];
            primitive_topologies  = (D3D_PRIMITIVE_TOPOLOGY*) &index_buffer_views[items_count];
            elements_types        = (u32*) &primitive_topologies[items_count];
            per_object_data       = (D3D12_GPU_VIRTUAL_ADDRESS*) &elements_types[items_count];
        }
    }

private:
    constexpr static u32 struct_size{
        id::size +                          // entity_ids
        id::size +                          // submesh_ids
        id::size +                          // material_ids
        sizeof(ID3D12PipelineState*) +      // gpass_pipeline_states
        sizeof(ID3D12PipelineState*) +      // depth_pipeline_states
        sizeof(ID3D12RootSignature*) +      // root_signatures
        sizeof(material_type::type) +       // material_types
        sizeof(D3D12_GPU_VIRTUAL_ADDRESS) + // position_buffers
        sizeof(D3D12_GPU_VIRTUAL_ADDRESS) + // element_buffers
        sizeof(D3D12_INDEX_BUFFER_VIEW) +   // index_buffer_views
        sizeof(D3D_PRIMITIVE_TOPOLOGY) +    // primitive_topologies
        sizeof(u32) +                       // elements_types
        sizeof(D3D12_GPU_VIRTUAL_ADDRESS)   // per_object_data

    };

    utl::vector<u8> m_buffer;
} frame_cache;

#undef CONSTEXPR

bool create_buffers(vec2u size)
{
    assert(size.x && size.y);
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

void fill_per_object_data(const d3d12_frame_info& d3d12_info)
{
    const gpass_cache&   cache{ frame_cache };
    const u32            render_items_count{ cache.size() };
    id::id_type          current_entity_id{ id::invalid_id };
    hlsl::PerObjectData* current_data_ptr{ nullptr };

    constant_buffer& cbuffer{ core::cbuffer() };

    using namespace DirectX;
    for (u32 i{ 0 }; i < render_items_count; ++i)
    {
        if (current_entity_id != cache.entity_ids[i])
        {
            current_entity_id = cache.entity_ids[i];
            hlsl::PerObjectData data{};
            transform::get_transform_matrices(game_entity::entity_id{ current_entity_id }, data.World, data.InvWorld);
            const mat world{ XMLoadFloat4x4(&data.World) };
            const mat mvp{ XMMatrixMultiply(world, d3d12_info.camera->view_projection()) };
            XMStoreFloat4x4(&data.WorldViewProjection, mvp);

            current_data_ptr = cbuffer.allocate<hlsl::PerObjectData>();
            memcpy(current_data_ptr, &data, sizeof(hlsl::PerObjectData));
        }

        assert(current_data_ptr);
        cache.per_object_data[i] = cbuffer.gpu_address(current_data_ptr);
    }
}

void prepare_render_frame(const d3d12_frame_info& d3d12_info)
{
    assert(d3d12_info.info && d3d12_info.camera);
    assert(d3d12_info.info->render_item_ids && d3d12_info.info->render_item_count);
    gpass_cache& cache{ frame_cache };
    cache.clear();

    using namespace content;

    render_item::get_d3d12_render_item_ids(*d3d12_info.info, cache.d3d12_render_item_ids);
    cache.resize();
    const u32                      items_count{ cache.size() };
    const render_item::items_cache items_cache{ cache.items_cache() };
    render_item::get_items(cache.d3d12_render_item_ids.data(), items_count, items_cache);

    const submesh::views_cache views_cache{ cache.views_cache() };
    submesh::get_views(items_cache.submesh_gpu_ids, items_count, views_cache);

    const material::materials_cache materials_cache{ cache.materials_cache() };
    material::get_materials(items_cache.material_ids, items_count, materials_cache);

    fill_per_object_data(d3d12_info);
}


void set_root_parameters(id3d12_graphics_command_list* const cmd_list, u32 cache_index)
{
    const gpass_cache& cache{ frame_cache };
    assert(cache_index < cache.size());

    const material_type::type mtl_type{ cache.material_types[cache_index] };

    switch (mtl_type)
    {
    case material_type::opaque:
    {
        using params = opaque_root_parameter;
        cmd_list->SetGraphicsRootShaderResourceView(params::position_buffer, cache.position_buffers[cache_index]);
        cmd_list->SetGraphicsRootShaderResourceView(params::element_buffer, cache.element_buffers[cache_index]);
        cmd_list->SetGraphicsRootConstantBufferView(params::per_object_data, cache.per_object_data[cache_index]);
    }
    break;
    }
}

} // anonymous namespace

bool initialize()
{
    return create_buffers(initial_dimensions);
}

void shutdown()
{
    gpass_main_buffer.release();
    gpass_depth_buffer.release();

    dimensions = initial_dimensions;
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


void depth_prepass(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& d3d12_info)
{
    prepare_render_frame(d3d12_info);

    const gpass_cache& cache{ frame_cache };
    const u32          items_count{ cache.size() };

    ID3D12RootSignature* current_root_sig{ nullptr };
    ID3D12PipelineState* current_pipeline_state{ nullptr };

    for (u32 i{ 0 }; i < items_count; ++i)
    {
        if (current_root_sig != cache.root_signatures[i])
        {
            current_root_sig = cache.root_signatures[i];
            cmd_list->SetGraphicsRootSignature(current_root_sig);
            cmd_list->SetGraphicsRootConstantBufferView(opaque_root_parameter::global_shader_data, d3d12_info.global_shader_data);
        }

        if (current_pipeline_state != cache.depth_pipeline_states[i])
        {
            current_pipeline_state = cache.depth_pipeline_states[i];
            cmd_list->SetPipelineState(current_pipeline_state);
        }

        set_root_parameters(cmd_list, i);

        const D3D12_INDEX_BUFFER_VIEW& ibv{ cache.index_buffer_views[i] };
        const u32                      index_count{ ibv.SizeInBytes >> (ibv.Format == DXGI_FORMAT_R16_UINT ? 1 : 2) };

        cmd_list->IASetIndexBuffer(&ibv);
        cmd_list->IASetPrimitiveTopology(cache.primitive_topologies[i]);
        cmd_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
    }
}

void render(id3d12_graphics_command_list* cmd_list, const d3d12_frame_info& d3d12_info)
{
    const gpass_cache& cache{ frame_cache };
    const u32          items_count{ cache.size() };

    ID3D12RootSignature* current_root_sig{ nullptr };
    ID3D12PipelineState* current_pipeline_state{ nullptr };

    for (u32 i{ 0 }; i < items_count; ++i)
    {
        if (current_root_sig != cache.root_signatures[i])
        {
            current_root_sig = cache.root_signatures[i];
            cmd_list->SetGraphicsRootSignature(current_root_sig);
            cmd_list->SetGraphicsRootConstantBufferView(opaque_root_parameter::global_shader_data, d3d12_info.global_shader_data);
            cmd_list->SetGraphicsRootShaderResourceView(opaque_root_parameter::directional_lights,
                                                        light::non_cullable_light_buffer(d3d12_info.frame_index));
        }

        if (current_pipeline_state != cache.gpass_pipeline_states[i])
        {
            current_pipeline_state = cache.gpass_pipeline_states[i];
            cmd_list->SetPipelineState(current_pipeline_state);
        }

        set_root_parameters(cmd_list, i);

        const D3D12_INDEX_BUFFER_VIEW& ibv{ cache.index_buffer_views[i] };
        const u32                      index_count{ ibv.SizeInBytes >> (ibv.Format == DXGI_FORMAT_R16_UINT ? 1 : 2) };

        cmd_list->IASetIndexBuffer(&ibv);
        cmd_list->IASetPrimitiveTopology(cache.primitive_topologies[i]);
        cmd_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
    }
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
    barriers.add(gpass_main_buffer.resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET,
                 D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);

    barriers.add(gpass_depth_buffer.resource(),
                 D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
                 D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void add_transitions_gpass(d3dx::d3d12_resource_barrier& barriers)
{
    barriers.add(gpass_main_buffer.resource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET,
                 D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

    barriers.add(gpass_depth_buffer.resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE,
                 D3D12_RESOURCE_STATE_DEPTH_READ | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void add_transitions_post_process(d3dx::d3d12_resource_barrier& barriers)
{
    barriers.add(gpass_main_buffer.resource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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