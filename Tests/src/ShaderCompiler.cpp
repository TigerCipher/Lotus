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

#include <Lotus/Graphics/D3D12/D3D12Core.h>
#include <Lotus/Graphics/D3D12/D3D12Shaders.h>


#include "../../vendor/DirectXShaderCompiler/inc/d3d12shader.h"
#include "../../vendor/DirectXShaderCompiler/inc/dxcapi.h"

#include <filesystem>
#include <fstream>

#pragma comment(lib, "../../vendor/DirectXShaderCompiler/lib/x64/dxcompiler.lib")

using namespace lotus;
using namespace graphics::d3d12::shaders;

namespace
{

struct shader_file_info
{
    const char*       file;
    const char*       function;
    engine_shader::id id;
    shader_type::type type;
};

constexpr shader_file_info shader_files[]{
    {
        "FullscreenTriangle.hlsl",
        "FullscreenTriangleVS",
        engine_shader::fullscreen_triangle_vs,
        shader_type::vertex,
    },

    {
        "FillColor.hlsl",
        "FillColorPS",
        engine_shader::fillcolor_ps,
        shader_type::pixel,
    },
};

static_assert(_countof(shader_files) == engine_shader::count);

constexpr const char* shaders_src_path = "../../Lotus/src/Lotus/Graphics/D3D12/Shaders/";


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

    IDxcBlob* compile(shader_file_info info, std::filesystem::path full_path)
    {
        LASSERT(m_compiler && m_utils && m_include_handler);
        HRESULT hr{ S_OK };

        comptr<IDxcBlobEncoding> src_blob{ nullptr };

        DX_CALL(hr = m_utils->LoadFile(full_path.c_str(), nullptr, &src_blob));

        if (FAILED(hr))
            return nullptr;
        LASSERT(src_blob && src_blob->GetBufferSize());

        std::wstring file = utl::to_wstring(info.file);
        std::wstring func = utl::to_wstring(info.function);
        std::wstring prof = utl::to_wstring(m_profiles[info.type]);

        LPCWSTR args[]
        {
            file.c_str(),            // shader file name used for error reporting
                L"-E", func.c_str(), // Entrypoint function
                L"-T", prof.c_str(), // hlsl profile (i.e, vs_6_5)
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
        OutputDebugStringA(info.file);
        OutputDebugStringA(" =================\n");

        return compile(src_blob.Get(), args, _countof(args));
    }

    IDxcBlob* compile(IDxcBlobEncoding* src_blob, LPCWSTR* args, u32 num_args)
    {
        DxcBuffer buffer{};
        buffer.Encoding = DXC_CP_ACP;
        buffer.Ptr      = src_blob->GetBufferPointer();
        buffer.Size     = src_blob->GetBufferSize();

        HRESULT            hr{ S_OK };
        comptr<IDxcResult> results{ nullptr };
        DX_CALL(hr = m_compiler->Compile(&buffer, args, num_args, m_include_handler.Get(), L_PTR(&results)));
        if (FAILED(hr))
            return nullptr;

        comptr<IDxcBlobUtf8> errors{ nullptr };
        DX_CALL(hr = results->GetOutput(DXC_OUT_ERRORS, L_PTR(&errors), nullptr));
        if (FAILED(hr))
            return nullptr;

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
            return nullptr;

        comptr<IDxcBlob> shader{ nullptr };
        DX_CALL(hr = results->GetOutput(DXC_OUT_OBJECT, L_PTR(&shader), nullptr));
        if (FAILED(hr))
            return nullptr;

        return shader.Detach();
    }

private:
    comptr<IDxcCompiler3>      m_compiler{ nullptr };
    comptr<IDxcUtils>          m_utils{ nullptr };
    comptr<IDxcIncludeHandler> m_include_handler{ nullptr };

    constexpr static const char* m_profiles[]{ "vs_6_5", "hs_6_5", "ds_6_5", "gs_6_5",
                                               "ps_6_5", "cs_6_5", "as_6_5", "ms_6_5" };
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
    std::filesystem::path path{};
    std::filesystem::path full_path{};

    for (u32 i = 0; i < engine_shader::count; ++i)
    {
        auto& info = shader_files[i];
        path       = shaders_src_path;
        path += info.file;
        full_path = path;

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

bool save_compiled_shaders(utl::vector<comptr<IDxcBlob>>& shaders)
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
        const D3D12_SHADER_BYTECODE byte_code{ shader->GetBufferPointer(), shader->GetBufferSize() };
        file.write((char*) &byte_code.BytecodeLength, sizeof(byte_code.BytecodeLength));
        file.write((char*) byte_code.pShaderBytecode, byte_code.BytecodeLength);
    }

    file.close();

    return true;
}

} // anonymous namespace


bool compile_shaders()
{
    if (compiled_shaders_are_updated())
        return true;
    utl::vector<comptr<IDxcBlob>> shaders;

    std::filesystem::path path{};
    std::filesystem::path full_path{};


    shader_compiler compiler;
    for (const auto& info : shader_files)
    {
        path = shaders_src_path;
        path += info.file;
        full_path = path;

        if (!std::filesystem::exists(full_path))
            return false;

        comptr<IDxcBlob> compiled_shader{ compiler.compile(info, full_path) };

        if (compiled_shader && compiled_shader->GetBufferPointer() && compiled_shader->GetBufferSize())
        {
            shaders.emplace_back(std::move(compiled_shader));
        } else
        {
            return false;
        }
    }

    return save_compiled_shaders(shaders);
}
