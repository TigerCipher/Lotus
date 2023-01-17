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
    LASSERT(capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2);
    LASSERT(!(m_type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER && capacity > D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE));
    if (m_type == D3D12_DESCRIPTOR_HEAP_TYPE_DSV || m_type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV)
        is_shader_visible = false;

    release();

    ID3D12Device* const device = core::device();
    LASSERT(device);

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Flags = is_shader_visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
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
        LASSERT(m_deferred_free_indices[i].empty());
    }
#endif

    m_descriptor_size = device->GetDescriptorHandleIncrementSize(m_type);
    m_cpu_start       = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpu_start = is_shader_visible ? m_heap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{ 0 };

    return true;
}

void descriptor_heap::process_deferred_free(u32 frame_index)
{
    std::lock_guard lock(m_mutex);
    LASSERT(frame_index < frame_buffer_count);

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
    LASSERT(!m_size);
    core::deferred_release(m_heap);
}

descriptor_handle descriptor_heap::allocate()
{
    std::lock_guard lock(m_mutex);
    LASSERT(m_heap);
    LASSERT(m_size < m_capacity);

    const u32 index  = m_free_handles[m_size];
    const u32 offset = index * m_descriptor_size;
    ++m_size;
    descriptor_handle handle;
    handle.cpu.ptr = m_cpu_start.ptr + offset;
    if (is_shader_visible())
    {
        handle.gpu.ptr = m_gpu_start.ptr + offset;
    }
    L_DBG(handle.container = this);
    L_DBG(handle.index = index);
    return handle;
}

void descriptor_heap::free(descriptor_handle& handle)
{
    if (!handle.is_valid())
        return;
    std::lock_guard lock(m_mutex);
    LASSERT(m_heap && m_size);
    LASSERT(handle.container == this);
    LASSERT(handle.cpu.ptr >= m_cpu_start.ptr);
    LASSERT((handle.cpu.ptr - m_cpu_start.ptr) % m_descriptor_size == 0);
    LASSERT(handle.index < m_capacity);
    const u32 index = (u32) (handle.cpu.ptr - m_cpu_start.ptr) / m_descriptor_size;
    LASSERT(handle.index == index);

    const u32 frame_index = core::current_frame_index();
    m_deferred_free_indices[frame_index].push_back(index);
    core::set_derferred_releases_flag();

    handle = {};
}
}