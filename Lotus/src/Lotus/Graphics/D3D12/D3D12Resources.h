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
// File Name: D3D12Resources.h
// Date File Created: 01/16/2023
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "D3D12Common.h"

namespace lotus::graphics::d3d12
{
class descriptor_heap;

struct descriptor_handle
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpu{};

    constexpr bool is_valid() const { return cpu.ptr != 0; }
    constexpr bool is_shader_visible() const { return gpu.ptr != 0; }

#ifdef L_DEBUG
    friend class descriptor_heap;
    descriptor_heap* container{ nullptr };
    u32              index{ invalid_id_u32 };
#endif
};

class descriptor_heap
{
public:
    explicit descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE type) : m_type(type) {}

    DISABLE_COPY_AND_MOVE(descriptor_heap);
    ~descriptor_heap() { LASSERT(!m_heap); }

    bool initialize(u32 capacity, bool is_shader_visible);
    void process_deferred_free(u32 frame_index);
    void release();

    [[nodiscard]] descriptor_handle allocate();
    void                            free(descriptor_handle& handle);

    constexpr D3D12_DESCRIPTOR_HEAP_TYPE  type() const { return m_type; }
    constexpr D3D12_CPU_DESCRIPTOR_HANDLE cpu_start() const { return m_cpu_start; }
    constexpr D3D12_GPU_DESCRIPTOR_HANDLE gpu_start() const { return m_gpu_start; }
    constexpr ID3D12DescriptorHeap* const heap() const { return m_heap; }
    constexpr u32                         capacity() const { return m_capacity; }
    constexpr u32                         size() const { return m_size; }
    constexpr u32                         descriptor_size() const { return m_descriptor_size; }
    constexpr bool                        is_shader_visible() const { return m_gpu_start.ptr != 0; }

private:
    ID3D12DescriptorHeap*            m_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE      m_cpu_start{};
    D3D12_GPU_DESCRIPTOR_HANDLE      m_gpu_start{};
    Scope<u32[]>                     m_free_handles{};
    utl::vector<u32>                 m_deferred_free_indices[frame_buffer_count]{};
    std::mutex                       m_mutex;
    u32                              m_capacity{ 0 };
    u32                              m_size{ 0 };
    u32                              m_descriptor_size{};
    const D3D12_DESCRIPTOR_HEAP_TYPE m_type{};
};
} // namespace lotus::graphics::d3d12