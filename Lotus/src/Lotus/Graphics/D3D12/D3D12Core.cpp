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
#include "D3D12Resources.h"
#include "D3D12Surface.h"
#include "D3D12Helpers.h"

namespace lotus::graphics::d3d12::core
{

// TODO: This was just a testing function
void create_root_sig();
void create_a_sig_2();

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

        void release()
        {
            core::release(cmd_allocator);
            fence_value = 0;
        }

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

using surface_collection = utl::free_list<d3d12_surface>;

constexpr D3D_FEATURE_LEVEL min_feature_level    = D3D_FEATURE_LEVEL_11_0;
constexpr DXGI_FORMAT       render_target_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

ID3D12Device10*    main_device  = nullptr;
IDXGIFactory7*     dxgi_factory = nullptr;
d3d12_command      gfx_command;
surface_collection surfaces;

descriptor_heap rtv_desc_heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);         // render targets
descriptor_heap dsv_desc_heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);         // depth stencils
descriptor_heap srv_desc_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // shader resource views
descriptor_heap uav_desc_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // non shader visible

utl::vector<IUnknown*> deferred_releases[frame_buffer_count]{};
u32                    deferred_releases_flag[frame_buffer_count]{};
std::mutex             deferred_releases_mutex{};

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
    DX_CALL(D3D12CreateDevice(adapter, min_feature_level, L_PTR(&device)))
    DX_CALL(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &feat_level_info, sizeof(feat_level_info)))
    return feat_level_info.MaxSupportedFeatureLevel;
}

void NO_INLINE process_deferred_releases(u32 frame_index)
{
    std::lock_guard lock(deferred_releases_mutex);
    deferred_releases_flag[frame_index] = 0;
    rtv_desc_heap.process_deferred_free(frame_index);
    dsv_desc_heap.process_deferred_free(frame_index);
    srv_desc_heap.process_deferred_free(frame_index);
    uav_desc_heap.process_deferred_free(frame_index);

    utl::vector<IUnknown*>& resources = deferred_releases[frame_index];
    if (!resources.empty())
    {
        for (auto& res : resources)
        {
            release(res);
        }
        resources.clear();
    }
}
} // namespace

namespace detail
{
void deferred_release(IUnknown* resource)
{
    const u32       frame_index = current_frame_index();
    std::lock_guard lock(deferred_releases_mutex);
    deferred_releases[frame_index].push_back(resource);
    set_derferred_releases_flag();
}
} // namespace detail


bool initialize()
{
    if (main_device)
        shutdown();

    u32 dxgi_factory_flags = 0;
#ifdef L_DEBUG
    {
        cptr<ID3D12Debug3> debug_interface;
        if (SUCCEEDED(D3D12GetDebugInterface(L_PTR(&debug_interface))))
        {
            debug_interface->EnableDebugLayer();
        } else
        {
            OutputDebugStringA("Warning: D3D12 Debug interface not available. Optional feature \"Graphics Tools\" "
                               "should be installed on this device.");
        }

        dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    }

#endif

    HRESULT hr = S_OK;
    DX_CALL(hr = CreateDXGIFactory2(dxgi_factory_flags, L_PTR(&dxgi_factory)));
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
    DX_CALL(hr = D3D12CreateDevice(main_adapter.Get(), max_feature_level, L_PTR(&main_device)))
    if (FAILED(hr))
        return failed_init();

#ifdef L_DEBUG
    {
        cptr<ID3D12InfoQueue> info_queue;
        DX_CALL(main_device->QueryInterface(IID_PPV_ARGS(&info_queue)));
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
    }
#endif

    bool res = true;
    res &= rtv_desc_heap.initialize(512, false);
    res &= dsv_desc_heap.initialize(512, false);
    res &= srv_desc_heap.initialize(4096, true);
    res &= uav_desc_heap.initialize(512, false);
    if (!res)
        return failed_init();

    new (&gfx_command) d3d12_command(main_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    if (!gfx_command.command_queue())
        return failed_init();

    NAME_D3D_OBJ(main_device, L"MAIN_DEVICE");
    NAME_D3D_OBJ(rtv_desc_heap.heap(), L"RTV Descriptor Heap");
    NAME_D3D_OBJ(dsv_desc_heap.heap(), L"DSV Descriptor Heap");
    NAME_D3D_OBJ(srv_desc_heap.heap(), L"SRV Descriptor Heap");
    NAME_D3D_OBJ(uav_desc_heap.heap(), L"UAV Descriptor Heap");

    // TODO: Just for testing
    create_root_sig();
    create_a_sig_2();

    return true;
}

void shutdown()
{
    gfx_command.release();

    for (u32 i = 0; i < frame_buffer_count; ++i)
    {
        process_deferred_releases(i);
    }

    release(dxgi_factory);

    rtv_desc_heap.release();
    dsv_desc_heap.release();
    srv_desc_heap.release();
    uav_desc_heap.release();

    process_deferred_releases(0);

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
        DX_CALL(main_device->QueryInterface(L_PTR(&debug_device)));
        release(main_device);
        DX_CALL(
            debug_device->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL));
    }
#endif

    release(main_device);
}


ID3D12Device10* const device()
{
    return main_device;
}

u32 current_frame_index()
{
    return gfx_command.frame_index();
}

void set_derferred_releases_flag()
{
    deferred_releases_flag[current_frame_index()] = 1;
}

descriptor_heap& rtv_heap()
{
    return rtv_desc_heap;
}

descriptor_heap& dsv_heap()
{
    return dsv_desc_heap;
}

descriptor_heap& srv_heap()
{
    return srv_desc_heap;
}

descriptor_heap& uav_heap()
{
    return uav_desc_heap;
}

DXGI_FORMAT default_render_target_format()
{
    return render_target_format;
}

surface create_surface(platform::window window)
{
    surface_id id{ surfaces.add(window) };
    surfaces[id].create_swap_chain(dxgi_factory, gfx_command.command_queue(), render_target_format);
    return surface{ id };
}

void remove_surface(surface_id id)
{
    gfx_command.flush();
    surfaces.remove(id);
}

void resize_surface(surface_id id, u32 width, u32 height)
{
    gfx_command.flush();
    surfaces[id].resize();
}

u32 surface_width(surface_id id)
{
    return surfaces[id].width();
}

u32 surface_height(surface_id id)
{
    return surfaces[id].height();
}

void render_surface(surface_id id)
{
    gfx_command.begin_frame();
    ID3D12GraphicsCommandList6* cmd_list = gfx_command.command_list();

    const u32 frame_index = current_frame_index();
    if (deferred_releases_flag[frame_index])
    {
        process_deferred_releases(frame_index);
    }

    const d3d12_surface& surface{ surfaces[id] };
    surface.present();

    gfx_command.end_frame();
}

void create_root_sig()
{
    D3D12_ROOT_PARAMETER1 params[3];

    {
        auto& p         = params[0];
        p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        D3D12_ROOT_CONSTANTS consts{};
        consts.Num32BitValues = 2;
        consts.ShaderRegister = 0;
        consts.RegisterSpace  = 0;
        p.Constants           = consts;
        p.ShaderVisibility    = D3D12_SHADER_VISIBILITY_PIXEL;
    }

    {
        auto& p         = params[1];
        p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;

        D3D12_ROOT_DESCRIPTOR1 root_desc{};
        root_desc.Flags          = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
        root_desc.RegisterSpace  = 0;
        root_desc.ShaderRegister = 0;
        p.Descriptor             = root_desc;
        p.ShaderVisibility       = D3D12_SHADER_VISIBILITY_VERTEX;
    }

    {
        auto& p         = params[2];
        p.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        D3D12_ROOT_DESCRIPTOR_TABLE1 table{};
        table.NumDescriptorRanges = 1;
        D3D12_DESCRIPTOR_RANGE1 range{};
        range.RangeType                         = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors                    = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        range.Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        range.BaseShaderRegister                = 0;
        range.RegisterSpace                     = 0;
        table.pDescriptorRanges                 = &range;
        p.DescriptorTable                       = table;
        p.ShaderVisibility                      = D3D12_SHADER_VISIBILITY_PIXEL;
    }

    D3D12_STATIC_SAMPLER_DESC sampler_desc{};
    sampler_desc.AddressU         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressV         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.AddressW         = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler_desc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC1 desc{};

    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                 D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                 D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                 D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
                 D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
    desc.NumParameters     = _countof(params);
    desc.pParameters       = &params[0];
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers   = &sampler_desc;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rs_desc{};
    rs_desc.Version  = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rs_desc.Desc_1_1 = desc;

    HRESULT   hr{ S_OK };
    ID3DBlob* root_sig_blob{ nullptr };
    ID3DBlob* err_blob{ nullptr };

    if (FAILED(hr = D3D12SerializeVersionedRootSignature(&rs_desc, &root_sig_blob, &err_blob)))
    {
        L_DBG(const char* errmsg = err_blob ? (const char*) err_blob->GetBufferPointer() : "");
        L_DBG(OutputDebugStringA(errmsg));
        return;
    }

    LASSERT(root_sig_blob);
    ID3D12RootSignature* root_sig{ nullptr };
    DX_CALL(hr = device()->CreateRootSignature(0, root_sig_blob->GetBufferPointer(), root_sig_blob->GetBufferSize(),
                                               L_PTR(&root_sig)));

    release(root_sig_blob);
    release(err_blob);

    release(root_sig);
}

void create_a_sig_2()
{
    d3dx::d3d12_descriptor_range range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND, 0 };
    d3dx::d3d12_root_parameter   params[3]{};
    params[0].as_constants(2, D3D12_SHADER_VISIBILITY_PIXEL, 0);
    params[1].as_cbv(D3D12_SHADER_VISIBILITY_PIXEL, 1);
    params[2].as_descriptor_table(D3D12_SHADER_VISIBILITY_PIXEL, &range, 1);

    const d3dx::d3d12_root_signature_desc desc{ &params[0], _countof(params) };

    ID3D12RootSignature* root_sig = desc.create();

    release(root_sig);
}

ID3D12RootSignature*  root_signature;
D3D12_SHADER_BYTECODE vs{};

template<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE Type, typename T>
class alignas(void*) d3d12_pipeline_state_subobject
{
public:
    d3d12_pipeline_state_subobject() = default;
    constexpr explicit d3d12_pipeline_state_subobject(T subobject) : m_subobject{ subobject } {}
    d3d12_pipeline_state_subobject& operator=(const T& subobject)
    {
        m_subobject = subobject;
        return *this;
    }

private:
    const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE m_type{ Type };
    T                                         m_subobject{};
};

using d3d12_pipeline_state_subobject_root_signature =
    d3d12_pipeline_state_subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE, ID3D12RootSignature*>;
using d3d12_pipeline_state_subobject_vs =
    d3d12_pipeline_state_subobject<D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS, D3D12_SHADER_BYTECODE>;

void create_a_pipeline_state_object()
{
    struct
    {
        struct alignas(void*)
        {
            const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type{ D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_ROOT_SIGNATURE };
            ID3D12RootSignature*                      root_signature;
        } root_sig;
        struct alignas(void*)
        {
            const D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type{ D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VS };
            D3D12_SHADER_BYTECODE                     vs_code{};
        } vs;
    } stream;

    stream.root_sig.root_signature = root_signature;
    stream.vs.vs_code              = vs;

    D3D12_PIPELINE_STATE_STREAM_DESC desc{};
    desc.pPipelineStateSubobjectStream = &stream;
    desc.SizeInBytes                   = sizeof(stream);

    ID3D12PipelineState* pso{ nullptr };
    device()->CreatePipelineState(&desc, L_PTR(&pso));

    release(pso);
}

void create_a_pipeline_state_object2()
{
    struct
    {
        d3d12_pipeline_state_subobject_root_signature root_sig{ root_signature };
        d3d12_pipeline_state_subobject_vs vs_{vs};
    } stream;

    auto pso = d3dx::create_pipeline_state(&stream, sizeof(stream));

    release(pso);
}

} // namespace lotus::graphics::d3d12::core