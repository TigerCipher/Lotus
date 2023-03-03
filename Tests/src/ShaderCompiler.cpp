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
// File Name: ShaderCompiler.cpp
// Date File Created: 01/21/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "ShaderCompiler.h"

#include <d3d12shader.h>
#include <dxcapi.h>


#include <filesystem>
#include <fstream>

#include <Lotus/Graphics/D3D12/D3D12Core.h>
#include <Lotus/Graphics/D3D12/D3D12Shaders.h>
#include <Lotus/Content/ContentToEngine.h>
#include <Lotus/Util/IOStream.h>

// #pragma comment(lib, "../../vendor/DirectXShaderCompiler/lib/x64/dxcompiler.lib")

using namespace lotus;
using namespace graphics::d3d12::shaders;

namespace
{
// struct shader_file_info
// {
//     const char*       file;
//     const char*       function;
//     engine_shader::id id;
//     shader_type::type type;
// };

constexpr const char* shaders_src_path = "../../Lotus/src/Lotus/Graphics/D3D12/Shaders/";

struct engine_shader_info
{
    engine_shader::id id;
    shader_file_info  info;
};

constexpr engine_shader_info engine_shader_files[]{
    engine_shader::fullscreen_triangle_vs,
    {
        "FullscreenTriangle.hlsl",
        "FullscreenTriangleVS",
        shader_type::vertex,
    },

    engine_shader::fillcolor_ps,
    {
        "FillColor.hlsl",
        "FillColorPS",
        shader_type::pixel,
    },

    engine_shader::post_process_ps,
    {
        "PostProcess.hlsl",
        "PostProcessPS",
        shader_type::pixel,
    },
};

static_assert(_countof(engine_shader_files) == engine_shader::count);


struct dxc_compiled_shader
{
    comptr<IDxcBlob>     byte_code;
    comptr<IDxcBlobUtf8> disassembly;
    DxcShaderHash        hash;
};

class shader_compiler
{
public:
    shader_compiler()
    {
        HRESULT hr{ S_OK };
        DX_CALL(hr = DxcCreateInstance(CLSID_DxcCompiler, L_PTR(&m_compiler)));
        if (FAILED(hr))
            return;
        DX_CALL(hr = DxcCreateInstance(CLSID_DxcUtils, L_PTR(&m_utils)));
        if (FAILED(hr))
            return;
        DX_CALL(hr = m_utils->CreateDefaultIncludeHandler(&m_include_handler));
    }

    DISABLE_COPY_AND_MOVE(shader_compiler);

    dxc_compiled_shader compile(shader_file_info info, std::filesystem::path full_path)
    {
        LASSERT(m_compiler && m_utils && m_include_handler);
        HRESULT hr{ S_OK };

        comptr<IDxcBlobEncoding> src_blob{ nullptr };

        DX_CALL(hr = m_utils->LoadFile(full_path.c_str(), nullptr, &src_blob));

        if (FAILED(hr))
            return {};
        LASSERT(src_blob && src_blob->GetBufferSize());

        std::wstring file = utl::to_wstring(info.file_name);
        std::wstring func = utl::to_wstring(info.function);
        std::wstring prof = utl::to_wstring(m_profiles[info.type]);
        std::wstring inc  = utl::to_wstring(shaders_src_path);

        LPCWSTR args[]
        {
            file.c_str(),               // shader file name used for error reporting
                L"-E", func.c_str(),    // Entrypoint function
                L"-T", prof.c_str(),    // hlsl profile (i.e, vs_6_5)
                L"-I", inc.c_str(),     // include path
                L"-enable-16bit-types", // 16 bit integer support
                DXC_ARG_ALL_RESOURCES_BOUND,
#if L_DEBUG
                DXC_ARG_DEBUG, DXC_ARG_SKIP_OPTIMIZATIONS,
#else
                DXC_ARG_OPTIMIZATION_LEVEL3,
#endif
                DXC_ARG_WARNINGS_ARE_ERRORS,
                L"-Qstrip_reflect", // reflections go in separate blob
                L"-Qstrip_debug",   // debug info goes in separate blob
        };

        OutputDebugStringA("======== Compiling ");
        OutputDebugStringA(info.file_name);
        OutputDebugStringA(" : ");
        OutputDebugStringA(info.function);
        OutputDebugStringA("\n");

        return compile(src_blob.Get(), args, _countof(args));
    }

    dxc_compiled_shader compile(IDxcBlobEncoding* src_blob, LPCWSTR* args, u32 num_args)
    {
        DxcBuffer buffer{};
        buffer.Encoding = DXC_CP_ACP;
        buffer.Ptr      = src_blob->GetBufferPointer();
        buffer.Size     = src_blob->GetBufferSize();

        HRESULT            hr{ S_OK };
        comptr<IDxcResult> results{ nullptr };
        DX_CALL(hr = m_compiler->Compile(&buffer, args, num_args, m_include_handler.Get(), L_PTR(&results)));
        if (FAILED(hr))
            return {};

        comptr<IDxcBlobUtf8> errors{ nullptr };
        DX_CALL(hr = results->GetOutput(DXC_OUT_ERRORS, L_PTR(&errors), nullptr));
        if (FAILED(hr))
            return {};

        if (errors && errors->GetStringLength())
        {
            OutputDebugStringA("\n====== Shader Compilation Error ====================\n");
            OutputDebugStringA(errors->GetStringPointer());
            OutputDebugStringA("\n====================================================\n");
        } else
        {
            OutputDebugStringA("============ Shader Compilation Succeeded =============");
        }

        OutputDebugStringA("\n");

        HRESULT status{ S_OK };
        DX_CALL(hr = results->GetStatus(&status));
        if (FAILED(hr) || FAILED(status))
            return {};

        comptr<IDxcBlob> hash {nullptr};
        DX_CALL(hr = results->GetOutput(DXC_OUT_SHADER_HASH, L_PTR(&hash), nullptr));
        DxcShaderHash* const hash_buffer = (DxcShaderHash* const) hash->GetBufferPointer();
        LASSERT(!(hash_buffer->Flags & DXC_HASHFLAG_INCLUDES_SOURCE));
        OutputDebugStringA("==== Shader Hash: ");
        for (u32 i = 0; i < _countof(hash_buffer->HashDigest); ++i)
        {
            char hash_bytes[3]{};
            sprintf_s(hash_bytes, "%02x", (u32)hash_buffer->HashDigest[i]);
            OutputDebugStringA(hash_bytes);
            OutputDebugStringA(" ");
        }
        OutputDebugStringA("\n");

        comptr<IDxcBlob> shader{ nullptr };
        DX_CALL(hr = results->GetOutput(DXC_OUT_OBJECT, L_PTR(&shader), nullptr));
        if (FAILED(hr))
            return {};

        buffer.Ptr = shader->GetBufferPointer();
        buffer.Size = shader->GetBufferSize();

        comptr<IDxcResult> disasm_results{ nullptr };
        DX_CALL(hr = m_compiler->Disassemble(&buffer, L_PTR(&disasm_results)));

        comptr<IDxcBlobUtf8> disassembly{ nullptr };
        DX_CALL(hr = disasm_results->GetOutput(DXC_OUT_DISASSEMBLY, L_PTR(&disassembly), nullptr));

        dxc_compiled_shader result{shader.Detach(), disassembly.Detach()};
        memcpy(&result.hash.HashDigest[0], &hash_buffer->HashDigest[0], _countof(hash_buffer->HashDigest));

        return result;
    }

private:
    comptr<IDxcCompiler3>      m_compiler{ nullptr };
    comptr<IDxcUtils>          m_utils{ nullptr };
    comptr<IDxcIncludeHandler> m_include_handler{ nullptr };

    constexpr static const char* m_profiles[]{
        "vs_6_6", "hs_6_6", "ds_6_6", "gs_6_6", "ps_6_6", "cs_6_6", "as_6_6", "ms_6_6"
    };
    static_assert(_countof(m_profiles) == shader_type::count);
};

decltype(auto) get_engine_shaders_path()
{
    return std::filesystem::path{ graphics::get_engine_shaders_path(graphics::graphics_platform::d3d12) };
}

bool compiled_shaders_are_updated()
{
    const auto engine_shaders_path = get_engine_shaders_path();
    if (!std::filesystem::exists(engine_shaders_path))
        return false;
    const auto            comp_time = std::filesystem::last_write_time(engine_shaders_path);
    std::filesystem::path full_path{};

    for (u32 i = 0; i < engine_shader::count; ++i)
    {
        auto& info = engine_shader_files[i];
        full_path       = shaders_src_path;
        full_path += info.info.file_name;

        if (!std::filesystem::exists(full_path))
            return false;

        auto shader_file_time = std::filesystem::last_write_time(full_path);
        if (shader_file_time > comp_time)
        {
            return false;
        }
    }

    return true;
}

bool save_compiled_shaders(utl::vector<dxc_compiled_shader>& shaders)
{
    const auto engine_shaders_path = get_engine_shaders_path();
    std::filesystem::create_directories(engine_shaders_path.parent_path());

    std::ofstream file(engine_shaders_path, std::ios::out | std::ios::binary);
    if (!file || !std::filesystem::exists(engine_shaders_path))
    {
        file.close();
        return false;
    }

    for (auto& shader : shaders)
    {
        const D3D12_SHADER_BYTECODE byte_code{ shader.byte_code->GetBufferPointer(), shader.byte_code->GetBufferSize() };
        file.write((char*) &byte_code.BytecodeLength, sizeof(byte_code.BytecodeLength));
        file.write((char*) &shader.hash.HashDigest[0], _countof(shader.hash.HashDigest));
        file.write((char*) byte_code.pShaderBytecode, byte_code.BytecodeLength);
    }

    file.close();

    return true;
}

} // anonymous namespace


scope<u8[]> compile_shader(shader_file_info info, const char* file_path)
{
    std::filesystem::path full_path(file_path);
    full_path += info.file_name;
    if(!std::filesystem::exists(full_path)) return {};

    shader_compiler compiler{};
    dxc_compiled_shader compiled_shader{compiler.compile(info, full_path)};

    if (compiled_shader.byte_code && compiled_shader.byte_code->GetBufferPointer() &&
        compiled_shader.byte_code->GetBufferSize())
    {
        static_assert(content::compiled_shader::hash_length == _countof(DxcShaderHash::HashDigest));
        const u64 buffer_size = sizeof(u64) + content::compiled_shader::hash_length + compiled_shader.byte_code->GetBufferSize();
        scope<u8[]> buffer = create_scope<u8[]>(buffer_size);
        utl::blob_stream_writer blob(buffer.get(), buffer_size);
        blob.write(compiled_shader.byte_code->GetBufferSize());
        blob.write(compiled_shader.hash.HashDigest, content::compiled_shader::hash_length);
        blob.write((u8*)compiled_shader.byte_code->GetBufferPointer(), compiled_shader.byte_code->GetBufferSize());

        LASSERT(blob.offset() == buffer_size);
        return buffer;
    }

    return {};
}

bool compile_shaders()
{
    if (compiled_shaders_are_updated())
        return true;
    utl::vector<dxc_compiled_shader> shaders;

    std::filesystem::path full_path{};


    shader_compiler compiler;
    for (const auto& info : engine_shader_files)
    {
        full_path = shaders_src_path;
        full_path += info.info.file_name;

        if (!std::filesystem::exists(full_path))
            return false;

        dxc_compiled_shader compiled_shader{ compiler.compile(info.info, full_path) };

        if (compiled_shader.byte_code && compiled_shader.byte_code->GetBufferPointer() && compiled_shader.byte_code->GetBufferSize())
        {
            shaders.emplace_back(std::move(compiled_shader));
        } else
        {
            return false;
        }
    }

    return save_compiled_shaders(shaders);
}
