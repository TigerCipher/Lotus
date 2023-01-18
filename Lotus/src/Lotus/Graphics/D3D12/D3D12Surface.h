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
// File Name: D3D12Surface.h
// Date File Created: 01/17/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#pragma once

#include "D3D12Common.h"
#include "D3D12Resources.h"

namespace lotus::graphics::d3d12
{
class d3d12_surface
{
public:
    explicit d3d12_surface(platform::window window) : m_window(window) { LASSERT(m_window.handle()); }

    ~d3d12_surface() { release(); }

#if USE_STL_VECTOR
    DISABLE_COPY(d3d12_surface);
    constexpr d3d12_surface(d3d12_surface&& o) :
        m_window(o.m_window),
        m_swap_chan(o.m_swap_chan),
        m_current_backbuffer_index(o.m_current_backbuffer_index),
        m_viewport(o.m_viewport),
        m_scissor_rect(o.m_scissor_rect),
        m_allow_tearing(o.m_allow_tearing),
        m_present_flags(o.m_present_flags)
    {
        for (u32 i = 0; i < frame_buffer_count; ++i)
        {
            m_render_target_data[i].resource = o.m_render_target_data[i].resource;
            m_render_target_data[i].rtv      = o.m_render_target_data[i].rtv;
        }
        o.reset();
    }

    constexpr d3d12_surface& operator=(d3d12_surface&& o)
    {
        LASSERT(this != &o);
        if (this != &o)
        {
            release();
            move(o);
        }
        return *this;
    }
#else
    DISABLE_COPY_AND_MOVE(d3d12_surface);
#endif

    void create_swap_chain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue, DXGI_FORMAT format);
    void present() const;
    void resize();

    constexpr u32 width() const
    {
        return (u32) m_viewport.Width;
    }
    constexpr u32 height() const
    {
        return (u32) m_viewport.Height;
    }
    constexpr ID3D12Resource* const back_buffer() const
    {
        return m_render_target_data[m_current_backbuffer_index].resource;
    }
    constexpr D3D12_CPU_DESCRIPTOR_HANDLE rtv() const
    {
        return m_render_target_data[m_current_backbuffer_index].rtv.cpu;
    }
    constexpr const D3D12_VIEWPORT& viewport() const
    {
        return m_viewport;
    }
    constexpr const D3D12_RECT& scissor_rect() const
    {
        return m_scissor_rect;
    }

private:
    void release();
    void finalize();

#if USE_STL_VECTOR
    constexpr void reset()
    {
        m_window    = {};
        m_swap_chan = nullptr;
        for (u32 i = 0; i < frame_buffer_count; ++i)
        {
            m_render_target_data[i] = {};
        }
        m_current_backbuffer_index = 0;
        m_viewport                 = {};
        m_scissor_rect             = {};
        m_allow_tearing            = 0;
        m_allow_tearing            = 0;
    }

    constexpr void move(d3d12_surface& o)
    {
        m_window    = o.m_window;
        m_swap_chan = o.m_swap_chan;
        for (u32 i = 0; i < frame_buffer_count; ++i)
        {
            m_render_target_data[i] = o.m_render_target_data[i];
        }
        m_current_backbuffer_index = o.m_current_backbuffer_index;
        m_viewport                 = o.m_viewport;
        m_scissor_rect             = o.m_scissor_rect;
        m_allow_tearing            = o.m_allow_tearing;
        m_present_flags            = o.m_present_flags;
    }
#endif

    struct render_target_data
    {
        ID3D12Resource*   resource{ nullptr };
        descriptor_handle rtv{};
    };

    platform::window   m_window{};
    IDXGISwapChain4*   m_swap_chan{ nullptr };
    render_target_data m_render_target_data[frame_buffer_count]{};
    mutable u32        m_current_backbuffer_index{ 0 };
    u32                m_allow_tearing{ 0 };
    u32                m_present_flags{ 0 };
    D3D12_VIEWPORT     m_viewport{};
    D3D12_RECT         m_scissor_rect{};
};
}