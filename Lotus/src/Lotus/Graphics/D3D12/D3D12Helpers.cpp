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
// File Name: D3D12Helpers.cpp
// Date File Created: 01/19/2023
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "D3D12Helpers.h"
#include "D3D12Core.h"
#include "D3D12Upload.h"

namespace lotus::graphics::d3d12::d3dx
{

namespace
{} // anonymous namespace

ID3D12RootSignature* create_root_signature(const D3D12_ROOT_SIGNATURE_DESC1& desc)
{
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned_desc{};
    versioned_desc.Version  = D3D_ROOT_SIGNATURE_VERSION_1_1;
    versioned_desc.Desc_1_1 = desc;

    comptr<ID3DBlob> signature_blob{ nullptr };
    comptr<ID3DBlob> err_blob{ nullptr };

    L_HRES(hr);
    if (FAILED(hr = D3D12SerializeVersionedRootSignature(&versioned_desc, &signature_blob, &err_blob)))
    {
        L_DBG(const char* errmsg = err_blob ? (const char*) err_blob->GetBufferPointer() : "");
        L_DBG(OutputDebugStringA(errmsg));
        return nullptr;
    }

    ID3D12RootSignature* signature{ nullptr };
    DX_CALL(hr = core::device()->CreateRootSignature(0, signature_blob->GetBufferPointer(), signature_blob->GetBufferSize(),
                                                     L_PTR(&signature)));

    if (FAILED(hr))
    {
        core::release(signature);
    }

    return signature;
}

ID3D12PipelineState* create_pipeline_state(D3D12_PIPELINE_STATE_STREAM_DESC desc)
{
    assert(desc.pPipelineStateSubobjectStream && desc.SizeInBytes);
    ID3D12PipelineState* pso{ nullptr };
    DX_CALL(core::device()->CreatePipelineState(&desc, L_PTR(&pso)));
    assert(pso);
    return pso;
}

ID3D12PipelineState* create_pipeline_state(void* stream, u64 stream_size)
{
    assert(stream && stream_size);
    D3D12_PIPELINE_STATE_STREAM_DESC desc{};
    desc.SizeInBytes                   = stream_size;
    desc.pPipelineStateSubobjectStream = stream;
    return create_pipeline_state(desc);
}


ID3D12Resource* create_buffer(const void* data, u32 buffer_size, bool is_cpu_accessible, D3D12_RESOURCE_STATES state,
                              D3D12_RESOURCE_FLAGS flags, ID3D12Heap* heap, u64 heap_offset)
{
    assert(buffer_size);

    D3D12_RESOURCE_DESC desc{};
    desc.Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment        = 0;
    desc.Width            = buffer_size;
    desc.Height           = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels        = 1;
    desc.Format           = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc       = { 1, 0 };
    desc.Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags            = is_cpu_accessible ? D3D12_RESOURCE_FLAG_NONE : flags;

    assert(desc.Flags == D3D12_RESOURCE_FLAG_NONE || desc.Flags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    ID3D12Resource*             res       = nullptr;
    const D3D12_RESOURCE_STATES res_state = is_cpu_accessible ? D3D12_RESOURCE_STATE_GENERIC_READ : state;

    if (heap)
    {
        DX_CALL(core::device()->CreatePlacedResource(heap, heap_offset, &desc, res_state, nullptr, L_PTR(&res)));
    } else
    {
        DX_CALL(core::device()->CreateCommittedResource(is_cpu_accessible ? &heap_properties.upload_heap
                                                                          : &heap_properties.default_heap,
                                                        D3D12_HEAP_FLAG_NONE, &desc, res_state, nullptr, L_PTR(&res)));
    }

    if (data)
    {
        // Data that can be modified later will need to have is_cpu_accessible be true
        if (is_cpu_accessible)
        {
            D3D12_RANGE range{};
            void*       cpu_address = nullptr;
            DX_CALL(res->Map(0, &range, &cpu_address));
            assert(cpu_address);
            memcpy(cpu_address, data, buffer_size);
            res->Unmap(0, nullptr);
        } else
        {
            // GPU only accessible resources
            upload::d3d12_upload_context context(buffer_size);
            memcpy(context.cpu_address(), data, buffer_size);
            context.command_list()->CopyResource(res, context.upload_buffer());
            context.end_upload();
        }
    }

    assert(res);
    return res;
}

void transition_resource(id3d12_graphics_command_list* cmd_list, ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
                         D3D12_RESOURCE_STATES        after,
                         D3D12_RESOURCE_BARRIER_FLAGS flags /*= D3D12_RESOURCE_BARRIER_FLAG_NONE*/,
                         u32                          subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/)
{
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = flags;
    barrier.Transition.pResource   = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter  = after;
    barrier.Transition.Subresource = subresource;

    cmd_list->ResourceBarrier(1, &barrier);
}

} // namespace lotus::graphics::d3d12::d3dx
