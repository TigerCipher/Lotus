﻿// ------------------------------------------------------------------------------
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
// File Name: D3D12Resources.cpp
// Date File Created: 01/16/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "D3D12Resources.h"
#include "D3D12Core.h"

namespace lotus::graphics::d3d12
{
// DESCRIPTOR HEAP //////////////

bool descriptor_heap::initialize(u32 capacity, bool is_shader_visible)
{
    std::lock_guard lock(m_mutex);
    assert(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
    assert(!(m_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));
    if (m_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || m_type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        is_shader_visible = false;

    release();

    auto* const device = core::device();
    assert(device);

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Flags          = is_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NumDescriptors = capacity;
    desc.Type           = m_type;
    desc.NodeMask       = 0;

    HRESULT hr = S_OK;
    DX_CALL(hr = device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));
    if (FAILED(hr))
        return false;

    m_free_handles = std::move(create_scope<u32[]>(capacity));
    m_capacity     = capacity;
    m_size         = 0;

    for (u32 i = 0; i < capacity; ++i)
    {
        m_free_handles[i] = i;
    }

#ifdef L_DEBUG
    for (u32 i = 0; i < frame_buffer_count; ++i)
    {
        assert(m_deferred_free_indices[i].empty());
    }
#endif

    m_descriptor_size = device->GetDescriptorHandleIncrementSize(m_type);
    m_cpu_start       = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpu_start       = is_shader_visible ? m_heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

    return true;
}

void descriptor_heap::process_deferred_free(u32 frame_index)
{
    std::lock_guard lock(m_mutex);
    assert(frame_index < frame_buffer_count);

    utl::vector<u32>& indices = m_deferred_free_indices[frame_index];
    if (!indices.empty())
    {
        for (auto idx : indices)
        {
            --m_size;
            m_free_handles[m_size] = idx;
        }
        indices.clear();
    }
}

void descriptor_heap::release()
{
    assert(!m_size);
    core::deferred_release(m_heap);
}

descriptor_handle descriptor_heap::allocate()
{
    std::lock_guard lock(m_mutex);
    assert(m_heap);
    assert(m_size < m_capacity);

    const u32 index  = m_free_handles[m_size];
    const u32 offset = index * m_descriptor_size;
    ++m_size;
    descriptor_handle handle;
    handle.cpu.ptr = m_cpu_start.ptr + offset;
    if (is_shader_visible())
    {
        handle.gpu.ptr = m_gpu_start.ptr + offset;
    }
    handle.index = index;
    L_DBG(handle.container = this);
    return handle;
}

void descriptor_heap::free(descriptor_handle& handle)
{
    if (!handle.is_valid())
        return;
    std::lock_guard lock(m_mutex);
    assert(m_heap && m_size);
    assert(handle.container == this);
    assert(handle.cpu.ptr >= m_cpu_start.ptr);
    assert((handle.cpu.ptr - m_cpu_start.ptr) % m_descriptor_size == 0);
    assert(handle.index < m_capacity);
    const u32 index = (u32) (handle.cpu.ptr - m_cpu_start.ptr) / m_descriptor_size;
    assert(handle.index == index);

    const u32 frame_index = core::current_frame_index();
    m_deferred_free_indices[frame_index].push_back(index);
    core::set_derferred_releases_flag();

    handle = {};
}


// BUFFER //////////////

d3d12_buffer::d3d12_buffer(d3d12_buffer_init_info info, bool is_cpu_accessible)
{
    assert(info.size && info.alignment);
    m_size        = (u32) math::align_size_up(info.size, info.alignment);
    m_buffer      = d3dx::create_buffer(info.data, m_size, is_cpu_accessible, info.initial_state, info.flags, info.heap,
                                        info.allocation_info.Offset);
    m_gpu_address = m_buffer->GetGPUVirtualAddress();
    NAME_D3D_OBJ_INDEXED(m_buffer, m_size, L"D3D12 Buffer - Size");
}

void d3d12_buffer::release()
{
    core::deferred_release(m_buffer);
    m_gpu_address = 0;
    m_size        = 0;
}

// CONSTANT_BUFFER //////////////

constant_buffer::constant_buffer(d3d12_buffer_init_info info) : m_buffer{ info, true }
{
    NAME_D3D_OBJ_INDEXED(buffer(), size(), L"Constant Buffer - Size");

    const D3D12_RANGE range{};
    DX_CALL(buffer()->Map(0, &range, (void**) &m_cpu_address));
    assert(m_cpu_address);
}

u8* constant_buffer::allocate(u32 size)
{
    std::lock_guard lock{ m_mutex };

    const u32 aligned_size{ (u32) d3dx::align_size_for_constant_buffer(size) };
    assert(m_cpu_offset + aligned_size <= m_buffer.size());
    if (m_cpu_offset + aligned_size <= m_buffer.size())
    {
        u8* const address{ m_cpu_address + m_cpu_offset };
        m_cpu_offset += aligned_size;
        return address;
    }

    return nullptr;
}


// TEXTURE //////////////

d3d12_texture::d3d12_texture(d3d12_texture_init_info info)
{
    auto const device = core::device();
    assert(device);

    const D3D12_CLEAR_VALUE* const clear_value = info.desc && (info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET ||
                                                               info.desc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
                                                   ? &info.clear_value
                                                   : nullptr;

    if (info.resource)
    {
        assert(!info.heap);
        m_resource = info.resource;
    } else if (info.heap && info.desc)
    {
        assert(!info.resource);
        DX_CALL(device->CreatePlacedResource(info.heap, info.allocation_info.Offset, info.desc, info.initial_state, clear_value,
                                             L_PTR(&m_resource)));
    } else if (info.desc)
    {
        assert(!info.heap && !info.resource);


        DX_CALL(device->CreateCommittedResource(&d3dx::heap_properties.default_heap, D3D12_HEAP_FLAG_NONE, info.desc,
                                                info.initial_state, clear_value, L_PTR(&m_resource)));
    }

    assert(m_resource);

    m_srv = core::srv_heap().allocate();
    device->CreateShaderResourceView(m_resource, info.srv_desc, m_srv.cpu);
}

void d3d12_texture::release()
{
    core::srv_heap().free(m_srv);
    core::deferred_release(m_resource);
}


// RENDER TEXTURE //////////////


d3d12_render_texture::d3d12_render_texture(d3d12_texture_init_info info) : m_texture(info)
{
    assert(info.desc);
    m_mip_count = resource()->GetDesc().MipLevels;
    assert(m_mip_count && m_mip_count <= d3d12_texture::max_mips);

    descriptor_heap&              rtvheap = core::rtv_heap();
    D3D12_RENDER_TARGET_VIEW_DESC desc{};
    desc.Format             = info.desc->Format;
    desc.ViewDimension      = D3D12_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;

    auto* const device = core::device();
    assert(device);

    for (u32 i = 0; i < m_mip_count; ++i)
    {
        m_rtv[i] = rtvheap.allocate();
        device->CreateRenderTargetView(resource(), &desc, m_rtv[i].cpu);
        ++desc.Texture2D.MipSlice;
    }
}

void d3d12_render_texture::release()
{
    for (u32 i = 0; i < m_mip_count; ++i)
    {
        core::rtv_heap().free(m_rtv[i]);
    }
    m_texture.release();
    m_mip_count = 0;
}

// DEPTH BUFFER //////////////


d3d12_depth_buffer::d3d12_depth_buffer(d3d12_texture_init_info info)
{
    assert(info.desc);
    const DXGI_FORMAT               dsv_format = info.desc->Format;
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc{};
    if (info.desc->Format == DXGI_FORMAT_D32_FLOAT)
    {
        info.desc->Format = DXGI_FORMAT_R32_TYPELESS;
        srv_desc.Format   = DXGI_FORMAT_R32_FLOAT;
    }

    srv_desc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv_desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels           = 1;
    srv_desc.Texture2D.MostDetailedMip     = 0;
    srv_desc.Texture2D.PlaneSlice          = 0;
    srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;

    assert(!info.srv_desc && !info.resource);
    info.srv_desc = &srv_desc;
    m_texture     = d3d12_texture(info);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc{};
    dsv_desc.ViewDimension      = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_desc.Flags              = D3D12_DSV_FLAG_NONE;
    dsv_desc.Format             = dsv_format;
    dsv_desc.Texture2D.MipSlice = 0;

    m_dsv = core::dsv_heap().allocate();

    auto* const device = core::device();
    assert(device);

    device->CreateDepthStencilView(resource(), &dsv_desc, m_dsv.cpu);
}

void d3d12_depth_buffer::release()
{
    core::dsv_heap().free(m_dsv);
    m_texture.release();
}


} // namespace lotus::graphics::d3d12