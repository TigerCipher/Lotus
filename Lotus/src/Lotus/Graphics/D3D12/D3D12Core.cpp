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
// File Name: D3D12Core.cpp
// Date File Created: 10/30/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "D3D12Core.h"

namespace lotus::graphics::d3d12::core
{


namespace
{

class d3d12_command
{
public:
    d3d12_command() = default;
    DISABLE_COPY_AND_MOVE(d3d12_command);
    explicit d3d12_command(ID3D12Device8* const device, D3D12_COMMAND_LIST_TYPE type)
    {
        HRESULT                  hr = S_OK;
        D3D12_COMMAND_QUEUE_DESC desc;
        desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Type     = type;
        DX_CALL(hr = device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_cmd_queue)));
        if (FAILED(hr))
        {
            release();
            return;
        }
        NAME_D3D_OBJ(m_cmd_queue, type == D3D12_COMMAND_LIST_TYPE_DIRECT ? L"GFX Command Queue"
                                  : D3D12_COMMAND_LIST_TYPE_COMPUTE      ? L"Compute Command Queue"
                                                                         : L"Command Queue");

        for (u32 i = 0; i < frame_buffer_count; ++i)
        {
            command_frame& frame = m_cmd_frames[i];
            DX_CALL(hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&frame.cmd_allocator)));
            if (FAILED(hr))
            {
                release();
                return;
            }
            NAME_D3D_OBJ_INDEXED(frame.cmd_allocator, i,
                                 type == D3D12_COMMAND_LIST_TYPE_DIRECT ? L"GFX Command Allocator"
                                 : D3D12_COMMAND_LIST_TYPE_COMPUTE      ? L"Compute Command Allocator"
                                                                        : L"Command Allocator");
        }

        DX_CALL(
            hr = device->CreateCommandList(0, type, m_cmd_frames[0].cmd_allocator, nullptr, IID_PPV_ARGS(&m_cmd_list)));
        if (FAILED(hr))
        {
            release();
            return;
        }
        DX_CALL(m_cmd_list->Close());
        NAME_D3D_OBJ(m_cmd_list, type == D3D12_COMMAND_LIST_TYPE_DIRECT ? L"GFX Command List"
                                 : D3D12_COMMAND_LIST_TYPE_COMPUTE      ? L"Compute Command List"
                                                                        : L"Command List");

        DX_CALL(hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        if (FAILED(hr))
        {
            release();
            return;
        }
        NAME_D3D_OBJ(m_fence, L"D3D12 Fence");

        m_fence_event = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        LASSERT(m_fence_event);
    }

    ~d3d12_command() { LASSERT(!m_cmd_queue && !m_cmd_list && !m_fence); }

    void flush()
    {
        for (auto& cmd_frame : m_cmd_frames)
        {
            cmd_frame.wait(m_fence_event, m_fence);
        }
        m_frame_index = 0;
    }

    void release()
    {
        flush();
        core::release(m_fence);
        m_fence_value = 0;

        CloseHandle(m_fence_event);
        m_fence_event = nullptr;

        core::release(m_cmd_queue);
        core::release(m_cmd_list);

        for (auto& cmd_frame : m_cmd_frames)
        {
            cmd_frame.release();
        }
    }

    constexpr ID3D12CommandQueue* const         command_queue() const { return m_cmd_queue; }
    constexpr ID3D12GraphicsCommandList6* const command_list() const { return m_cmd_list; }
    constexpr u32                               frame_index() const { return m_frame_index; }

    void begin_frame()
    {
        command_frame& frame = m_cmd_frames[m_frame_index];
        frame.wait(m_fence_event, m_fence);
        DX_CALL(frame.cmd_allocator->Reset());
        DX_CALL(m_cmd_list->Reset(frame.cmd_allocator, nullptr));
    }

    void end_frame()
    {
        DX_CALL(m_cmd_list->Close());
        ID3D12CommandList* const cmd_lists[]{ m_cmd_list };
        m_cmd_queue->ExecuteCommandLists(_countof(cmd_lists), &cmd_lists[0]);
        u64& fence_value = m_fence_value;
        ++fence_value;
        command_frame& frame = m_cmd_frames[m_frame_index];
        frame.fence_value    = fence_value;
        m_cmd_queue->Signal(m_fence, fence_value);

        m_frame_index = (m_frame_index + 1) % frame_buffer_count;
    }

private:
    struct command_frame
    {
        ID3D12CommandAllocator* cmd_allocator = nullptr;
        u64                     fence_value   = 0;

        void release() { core::release(cmd_allocator); }
        void wait(HANDLE fence_event, ID3D12Fence1* fence)
        {
            LASSERT(fence && fence_event);
            if (fence->GetCompletedValue() < fence_value)
            {
                DX_CALL(fence->SetEventOnCompletion(fence_value, fence_event));
                WaitForSingleObject(fence_event, INFINITE);
            }
        }
    };

    ID3D12CommandQueue*         m_cmd_queue   = nullptr;
    ID3D12GraphicsCommandList6* m_cmd_list    = nullptr;
    ID3D12Fence1*               m_fence       = nullptr;
    u64                         m_fence_value = 0;
    command_frame               m_cmd_frames[frame_buffer_count]{};
    HANDLE                      m_fence_event = nullptr;
    u32                         m_frame_index = 0;
};


constexpr D3D_FEATURE_LEVEL min_feature_level = D3D_FEATURE_LEVEL_11_0;

ID3D12Device8* main_device  = nullptr;
IDXGIFactory7* dxgi_factory = nullptr;
d3d12_command  gfx_command;

bool failed_init()
{
    shutdown();
    return false;
}


IDXGIAdapter4* determine_main_adapter()
{
    IDXGIAdapter4* adapter = nullptr;
    // Get in order of performance
    for (u32 i = 0; dxgi_factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                             IID_PPV_ARGS(&adapter)) != DXGI_ERROR_NOT_FOUND;
         ++i)
    {
        // Pick the first that features minimum feature level
        if (SUCCEEDED(D3D12CreateDevice(adapter, min_feature_level, __uuidof(ID3D12Device), nullptr)))
        {
            return adapter;
        }

        release(adapter);
    }

    return nullptr;
}

D3D_FEATURE_LEVEL get_max_feature_level(IDXGIAdapter4* adapter)
{
    constexpr D3D_FEATURE_LEVEL feat_levels[4] = {
        D3D_FEATURE_LEVEL_11_0, //
        D3D_FEATURE_LEVEL_11_1, //
        D3D_FEATURE_LEVEL_12_0, //
        D3D_FEATURE_LEVEL_12_1, //
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS feat_level_info;
    feat_level_info.NumFeatureLevels        = _countof(feat_levels);
    feat_level_info.pFeatureLevelsRequested = feat_levels;

    cptr<ID3D12Device> device;
    DX_CALL(D3D12CreateDevice(adapter, min_feature_level, IID_PPV_ARGS(&device)))
    DX_CALL(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feat_level_info, sizeof(feat_level_info)))
    return feat_level_info.MaxSupportedFeatureLevel;
}

} // namespace


bool initialize()
{
    if (main_device)
        shutdown();

    u32 dxgi_factory_flags = 0;
#ifdef L_DEBUG
    {
        cptr<ID3D12Debug3> debug_interface;
        DX_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)));
        debug_interface->EnableDebugLayer();
        dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    }

#endif

    HRESULT hr = S_OK;
    DX_CALL(hr = CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));
    if (FAILED(hr))
        return failed_init();

    // Determine which adapter/gpu to use
    cptr<IDXGIAdapter4> main_adapter;
    main_adapter.Attach(determine_main_adapter());
    if (!main_adapter)
        return failed_init();

    // Determine maximum feature level
    const D3D_FEATURE_LEVEL max_feature_level = get_max_feature_level(main_adapter.Get());
    LASSERT(max_feature_level >= min_feature_level);
    if (max_feature_level < min_feature_level)
        return false;

    // Create d3d12 device (aka our virtual gpu)
    DX_CALL(hr = D3D12CreateDevice(main_adapter.Get(), max_feature_level, IID_PPV_ARGS(&main_device)))
    if (FAILED(hr))
        return failed_init();

    new (&gfx_command) d3d12_command(main_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (!gfx_command.command_queue())
        return failed_init();

    NAME_D3D_OBJ(main_device, L"MAIN_DEVICE");

#ifdef L_DEBUG
    {
        cptr<ID3D12InfoQueue> info_queue;
        DX_CALL(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
    }
#endif

    return true;
}

void shutdown()
{
    gfx_command.release();
    release(dxgi_factory);

#ifdef L_DEBUG
    {
        {
            cptr<ID3D12InfoQueue> info_queue;
            DX_CALL(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, false);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
            info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
        }

        cptr<ID3D12DebugDevice2> debug_device;
        DX_CALL(main_device->QueryInterface(IID_PPV_ARGS(&debug_device)));
        release(main_device);
        DX_CALL(
            debug_device->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
    }
#endif

    release(main_device);
}

void render()
{
    gfx_command.begin_frame();
    ID3D12GraphicsCommandList6* cmd_list = gfx_command.command_list();

    gfx_command.end_frame();
}
} // namespace lotus::graphics::d3d12::core
