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

    HRESULT hr{ S_OK };
    if (FAILED(hr = D3D12SerializeVersionedRootSignature(&versioned_desc, &signature_blob, &err_blob)))
    {
        L_DBG(const char* errmsg = err_blob ? (const char*) err_blob->GetBufferPointer() : "");
        L_DBG(OutputDebugStringA(errmsg));
        return nullptr;
    }

    ID3D12RootSignature* signature{ nullptr };
    DX_CALL(hr = core::device()->CreateRootSignature(0, signature_blob->GetBufferPointer(),
                                                     signature_blob->GetBufferSize(), L_PTR(&signature)));

    if (FAILED(hr))
    {
        core::release(signature);
    }

    return signature;
}

ID3D12PipelineState* create_pipeline_state(D3D12_PIPELINE_STATE_STREAM_DESC desc)
{
    LASSERT(desc.pPipelineStateSubobjectStream && desc.SizeInBytes);
    ID3D12PipelineState* pso{ nullptr };
    DX_CALL(core::device()->CreatePipelineState(&desc, L_PTR(&pso)));
    LASSERT(pso);
    return pso;
}

ID3D12PipelineState* create_pipeline_state(void* stream, u64 stream_size)
{
    LASSERT(stream && stream_size);
    D3D12_PIPELINE_STATE_STREAM_DESC desc{};
    desc.SizeInBytes                   = stream_size;
    desc.pPipelineStateSubobjectStream = stream;
    return create_pipeline_state(desc);
}
} // namespace lotus::graphics::d3d12::d3dx
