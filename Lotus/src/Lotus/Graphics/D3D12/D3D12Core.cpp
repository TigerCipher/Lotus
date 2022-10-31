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
constexpr D3D_FEATURE_LEVEL min_feature_level = D3D_FEATURE_LEVEL_11_0;

ID3D12Device8* main_device  = nullptr;
IDXGIFactory7* dxgi_factory = nullptr;

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
    // Determine maximum feature level
    // Create d3d12 device (aka our virtual gpu)

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

    const D3D_FEATURE_LEVEL max_feature_level = get_max_feature_level(main_adapter.Get());
    LASSERT(max_feature_level >= min_feature_level);
    if (max_feature_level < min_feature_level)
        return false;

    DX_CALL(hr = D3D12CreateDevice(main_adapter.Get(), max_feature_level, IID_PPV_ARGS(&main_device)))
    if (FAILED(hr))
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
        DX_CALL(debug_device->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));

    }
#endif

    release(main_device);
}
} // namespace lotus::graphics::d3d12::core
