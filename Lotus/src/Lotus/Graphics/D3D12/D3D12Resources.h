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

    [[nodiscard]] constexpr bool is_valid() const { return cpu.ptr != 0; }
    [[nodiscard]] constexpr bool is_shader_visible() const { return gpu.ptr != 0; }

    u32 index{ invalid_id_u32 };
#ifdef L_DEBUG
    friend class descriptor_heap;
    descriptor_heap* container{ nullptr };
#endif
};

class descriptor_heap
{
public:
    explicit descriptor_heap(D3D12_DESCRIPTOR_HEAP_TYPE type) : m_type(type) {}

    DISABLE_COPY_AND_MOVE(descriptor_heap);
    ~descriptor_heap() { assert(!m_heap); }

    bool initialize(u32 capacity, bool is_shader_visible);
    void process_deferred_free(u32 frame_index);
    void release();

    [[nodiscard]] descriptor_handle allocate();
    void                            free(descriptor_handle& handle);

    [[nodiscard]] constexpr D3D12_DESCRIPTOR_HEAP_TYPE  type() const { return m_type; }
    [[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE cpu_start() const { return m_cpu_start; }
    [[nodiscard]] constexpr D3D12_GPU_DESCRIPTOR_HANDLE gpu_start() const { return m_gpu_start; }
    [[nodiscard]] constexpr ID3D12DescriptorHeap* const heap() const { return m_heap; }
    [[nodiscard]] constexpr u32                         capacity() const { return m_capacity; }
    [[nodiscard]] constexpr u32                         size() const { return m_size; }
    [[nodiscard]] constexpr u32                         descriptor_size() const { return m_descriptor_size; }
    [[nodiscard]] constexpr bool                        is_shader_visible() const { return m_gpu_start.ptr != 0; }

private:
    ID3D12DescriptorHeap*            m_heap{ nullptr };
    D3D12_CPU_DESCRIPTOR_HANDLE      m_cpu_start{};
    D3D12_GPU_DESCRIPTOR_HANDLE      m_gpu_start{};
    scope<u32[]>                     m_free_handles{};
    utl::vector<u32>                 m_deferred_free_indices[frame_buffer_count]{};
    std::mutex                       m_mutex;
    u32                              m_capacity{ 0 };
    u32                              m_size{ 0 };
    u32                              m_descriptor_size{};
    const D3D12_DESCRIPTOR_HEAP_TYPE m_type{};
};

struct d3d12_buffer_init_info
{
    ID3D12Heap1*                    heap{ nullptr };
    const void*                     data{ nullptr };
    D3D12_RESOURCE_ALLOCATION_INFO1 allocation_info{};
    D3D12_RESOURCE_STATES           initial_state{};
    D3D12_RESOURCE_FLAGS            flags{ D3D12_RESOURCE_FLAG_NONE };
    u32                             size{ 0 };
    u32                             stride{ 0 };
    u32                             element_count{ 0 };
    u32                             alignment{ 0 };
    bool                            create_uav{ false };
};

class d3d12_buffer
{
public:
    d3d12_buffer() = default;
    explicit d3d12_buffer(d3d12_buffer_init_info info, bool is_cpu_accessible);
    DISABLE_COPY(d3d12_buffer);

    constexpr d3d12_buffer(d3d12_buffer&& o) noexcept :
        m_buffer{ o.m_buffer }, m_gpu_address{ o.m_gpu_address }, m_size{ o.m_size }
    {
        o.reset();
    }


    constexpr d3d12_buffer& operator=(d3d12_buffer&& o) noexcept
    {
        assert(this != &o);
        if (this != &o)
        {
            release();
            move(o);
        }

        return *this;
    }

    ~d3d12_buffer() { release(); }

    void release();

    [[nodiscard]] constexpr ID3D12Resource*           buffer() const { return m_buffer; }
    [[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address() const { return m_gpu_address; }
    [[nodiscard]] constexpr u32                       size() const { return m_size; }

private:
    constexpr void move(d3d12_buffer& o)
    {
        m_buffer      = o.m_buffer;
        m_gpu_address = o.m_gpu_address;
        m_size        = o.m_size;
        o.reset();
    }
    constexpr void reset()
    {
        m_buffer      = nullptr;
        m_gpu_address = 0;
        m_size        = 0;
    }
    ID3D12Resource*           m_buffer{ nullptr };
    D3D12_GPU_VIRTUAL_ADDRESS m_gpu_address{ 0 };
    u32                       m_size{};
};

class constant_buffer
{
public:
    constant_buffer() = default;
    explicit constant_buffer(d3d12_buffer_init_info info);
    DISABLE_COPY_AND_MOVE(constant_buffer);
    ~constant_buffer() { release(); }

    void release()
    {
        m_buffer.release();
        m_cpu_address = nullptr;
        m_cpu_offset  = 0;
    }

    constexpr void clear() { m_cpu_offset = 0; }

    [[nodiscard]] u8* allocate(u32 size);

    template<typename T>
    [[nodiscard]] T* allocate()
    {
        return (T* const) allocate(sizeof(T));
    }

    [[nodiscard]] constexpr ID3D12Resource*           buffer() const { return m_buffer.buffer(); }
    [[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address() const { return m_buffer.gpu_address(); }
    [[nodiscard]] constexpr u32                       size() const { return m_buffer.size(); }
    [[nodiscard]] constexpr u8*                       cpu_address() const { return m_cpu_address; }

    template<typename T>
    [[nodiscard]] constexpr D3D12_GPU_VIRTUAL_ADDRESS gpu_address(T* const allocation)
    {
        std::lock_guard lock{ m_mutex };
        assert(m_cpu_address);
        if (!m_cpu_address)
            return {};
        const auto address{ (const u8* const) allocation };
        assert(address <= m_cpu_address + m_cpu_offset);
        assert(address >= m_cpu_address);
        const u64 offset{ (u64) (address - m_cpu_address) };
        return m_buffer.gpu_address() + offset;
    }

    [[nodiscard]] constexpr static d3d12_buffer_init_info get_default_init_info(u32 size)
    {
        assert(size);
        d3d12_buffer_init_info info{};
        info.size      = size;
        info.alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        return info;
    }

private:
    d3d12_buffer m_buffer{};
    u8*          m_cpu_address{ nullptr };
    u32          m_cpu_offset{};
    std::mutex   m_mutex{};
};

struct d3d12_texture_init_info
{
    ID3D12Heap1*                     heap{ nullptr };
    ID3D12Resource*                  resource{ nullptr };
    D3D12_SHADER_RESOURCE_VIEW_DESC* srv_desc{ nullptr };
    D3D12_RESOURCE_DESC*             desc{ nullptr };
    D3D12_RESOURCE_ALLOCATION_INFO1  allocation_info{};
    D3D12_RESOURCE_STATES            initial_state{};
    D3D12_CLEAR_VALUE                clear_value{};
};

class d3d12_texture
{
public:
    constexpr static u32 max_mips{ 14 }; // supports 16k resolution
    d3d12_texture() = default;
    ~d3d12_texture() { release(); }
    explicit d3d12_texture(d3d12_texture_init_info info);
    DISABLE_COPY(d3d12_texture);

    constexpr d3d12_texture(d3d12_texture&& o) : m_resource(o.m_resource), m_srv(o.m_srv) { o.reset(); }

    constexpr d3d12_texture& operator=(d3d12_texture&& o)
    {
        assert(this != &o);
        if (this != &o)
        {
            release();
            move(o);
        }
        return *this;
    }

    void release();


    [[nodiscard]] constexpr ID3D12Resource* const resource() const { return m_resource; }
    [[nodiscard]] constexpr descriptor_handle     srv() const { return m_srv; }

private:
    constexpr void reset()
    {
        m_resource = nullptr;
        m_srv      = {};
    }

    constexpr void move(d3d12_texture& o)
    {
        m_resource = o.m_resource;
        m_srv      = o.m_srv;
        o.reset();
    }

    ID3D12Resource*   m_resource{ nullptr };
    descriptor_handle m_srv;
};

class d3d12_render_texture
{
public:
    d3d12_render_texture() = default;
    ~d3d12_render_texture() { release(); }
    explicit d3d12_render_texture(d3d12_texture_init_info info);
    DISABLE_COPY(d3d12_render_texture);

    constexpr d3d12_render_texture(d3d12_render_texture&& o) : m_texture(std::move(o.m_texture)), m_mip_count(o.m_mip_count)
    {
        for (u32 i = 0; i < m_mip_count; ++i)
        {
            m_rtv[i] = o.m_rtv[i];
        }
        o.reset();
    }

    constexpr d3d12_render_texture& operator=(d3d12_render_texture&& o)
    {
        assert(this != &o);
        if (this != &o)
        {
            release();
            move(o);
        }
        return *this;
    }

    void release();

    [[nodiscard]] constexpr u32 mip_count() const { return m_mip_count; }

    [[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv(u32 mip_idx) const
    {
        assert(mip_idx < m_mip_count);
        return m_rtv[mip_idx].cpu;
    }

    [[nodiscard]] constexpr descriptor_handle srv() const { return m_texture.srv(); }

    [[nodiscard]] constexpr ID3D12Resource* resource() const { return m_texture.resource(); }

private:
    constexpr void reset()
    {
        for (u32 i = 0; i < m_mip_count; ++i)
        {
            m_rtv[i] = {};
        }
        m_mip_count = 0;
    }

    constexpr void move(d3d12_render_texture& o)
    {
        m_texture   = std::move(o.m_texture);
        m_mip_count = o.m_mip_count;
        for (u32 i = 0; i < m_mip_count; ++i)
        {
            m_rtv[i] = o.m_rtv[i];
        }
        o.reset();
    }

    d3d12_texture     m_texture{};
    descriptor_handle m_rtv[d3d12_texture::max_mips]{};
    u32               m_mip_count{ 0 };
};

class d3d12_depth_buffer
{
public:
    d3d12_depth_buffer() = default;
    ~d3d12_depth_buffer() { release(); }
    explicit d3d12_depth_buffer(d3d12_texture_init_info info);
    DISABLE_COPY(d3d12_depth_buffer);

    constexpr d3d12_depth_buffer(d3d12_depth_buffer&& o) : m_texture(std::move(o.m_texture)), m_dsv(o.m_dsv) { o.m_dsv = {}; }

    constexpr d3d12_depth_buffer& operator=(d3d12_depth_buffer&& o)
    {
        assert(this != &o);
        if (this != &o)
        {
            m_texture = std::move(o.m_texture);
            m_dsv     = o.m_dsv;
            o.m_dsv   = {};
        }
        return *this;
    }

    void release();

    [[nodiscard]] constexpr D3D12_CPU_DESCRIPTOR_HANDLE dsv() const { return m_dsv.cpu; }
    [[nodiscard]] constexpr descriptor_handle           srv() const { return m_texture.srv(); }
    [[nodiscard]] constexpr ID3D12Resource*             resource() const { return m_texture.resource(); }

private:
    d3d12_texture     m_texture{};
    descriptor_handle m_dsv{};
};

} // namespace lotus::graphics::d3d12