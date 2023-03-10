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
// File Name: D3D12Surface.cpp
// Date File Created: 01/17/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "D3D12Surface.h"


#include "D3D12Core.h"

namespace lotus::graphics::d3d12
{
namespace
{
constexpr DXGI_FORMAT to_non_srgb(DXGI_FORMAT format)
{
    if (format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    return format;
}
} // anonymous namespace

void d3d12_surface::create_swap_chain(IDXGIFactory7* factory, ID3D12CommandQueue* cmd_queue, DXGI_FORMAT format)
{
    LASSERT(factory && cmd_queue);
    release();

    if (SUCCEEDED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &m_allow_tearing, sizeof(u32))) &&
        m_allow_tearing)
    {
        m_present_flags = DXGI_PRESENT_ALLOW_TEARING;
    }

    m_format = format;

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.BufferCount        = buffer_count;
    desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.Flags              = m_allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    desc.Format             = to_non_srgb(format);
    desc.Width              = m_window.width();
    desc.Height             = m_window.height();
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.Scaling            = DXGI_SCALING_STRETCH;
    desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Stereo             = false;

    IDXGISwapChain1* swap_chain;
    const auto       hwnd = (HWND) m_window.handle();
    DX_CALL(factory->CreateSwapChainForHwnd(cmd_queue, hwnd, &desc, nullptr, nullptr, &swap_chain));
    DX_CALL(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    DX_CALL(swap_chain->QueryInterface(L_PTR(&m_swap_chan)));
    core::release(swap_chain);

    m_current_backbuffer_index = m_swap_chan->GetCurrentBackBufferIndex();

    for (auto& [resource, rtv] : m_render_target_data)
    {
        rtv = core::rtv_heap().allocate();
    }

    finalize();
}

void d3d12_surface::present() const
{
    LASSERT(m_swap_chan);
    DX_CALL(m_swap_chan->Present(0, m_present_flags));
    m_current_backbuffer_index = m_swap_chan->GetCurrentBackBufferIndex();
}

void d3d12_surface::resize()
{
    LASSERT(m_swap_chan);
    for (u32 i = 0; i < buffer_count; ++i)
    {
        core::release(m_render_target_data[i].resource);
    }
    
    const u32 flags = m_allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0ul;
    DX_CALL(m_swap_chan->ResizeBuffers(buffer_count, 0, 0, DXGI_FORMAT_UNKNOWN, flags));
    m_current_backbuffer_index = m_swap_chan->GetCurrentBackBufferIndex();

    finalize();

    L_DBG(OutputDebugStringA("===== D3D12 Surface Resized\n"));
}

void d3d12_surface::release()
{
    for (auto& [resource, rtv] : m_render_target_data)
    {
        core::release(resource);
        core::rtv_heap().free(rtv);
    }

    core::release(m_swap_chan);
}

void d3d12_surface::finalize()
{
    for (u32 i = 0; i < buffer_count; ++i)
    {
        render_target_data& data = m_render_target_data[i];
        LASSERT(!data.resource);
        DX_CALL(m_swap_chan->GetBuffer(i, IID_PPV_ARGS(&data.resource)));
        D3D12_RENDER_TARGET_VIEW_DESC desc{};
        desc.Format        = m_format;
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        core::device()->CreateRenderTargetView(data.resource, &desc, data.rtv.cpu);
    }

    DXGI_SWAP_CHAIN_DESC desc{};
    DX_CALL(m_swap_chan->GetDesc(&desc));
    const u32 width  = desc.BufferDesc.Width;
    const u32 height = desc.BufferDesc.Height;
    LASSERT(m_window.width() == width && m_window.height() == height);

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width    = (f32) width;
    m_viewport.Height   = (f32) height;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_scissor_rect = { 0, 0, (i32) width, (i32) height };
}
} // namespace lotus::graphics::d3d12