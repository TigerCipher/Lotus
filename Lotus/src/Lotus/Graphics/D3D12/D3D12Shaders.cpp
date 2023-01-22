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
// File Name: D3D12Shaders.cpp
// Date File Created: 01/21/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "D3D12Shaders.h"
#include "../../Content/ContentLoader.h"

namespace lotus::graphics::d3d12::shaders
{

namespace
{

typedef struct compiled_shader
{
    u64         size{};
    const byte* byte_code;
} const* compiled_shader_ptr;

compiled_shader_ptr engine_shaders[engine_shader::count]{};

// memory containing all compiled shaders in format of size->bytecode->size->bytecode...
scope<byte[]> shaders_blob{};

bool load_engine_shaders()
{
    LASSERT(!shaders_blob);
    u64 size = 0;

    bool result = content::load_engine_shaders(shaders_blob, size);
    LASSERT(shaders_blob && size);

    u64 offset = 0;
    u32 idx = 0;
    while(offset < size && result)
    {
        LASSERT(idx < engine_shader::count);
        compiled_shader_ptr& shader = engine_shaders[idx];
        LASSERT(!shader);
        result &= idx < engine_shader::count && !shader;
        if (!result) break;
        shader = reinterpret_cast<const compiled_shader_ptr>(&shaders_blob[offset]);
        offset += sizeof(u64) + shader->size;
        ++idx;
    }

    LASSERT(offset == size && idx == engine_shader::count);

    return result;
}

} // anonymous namespace

bool initialize()
{
    return load_engine_shaders();
}

void shutdown()
{
    for (u32 i = 0; i < engine_shader::count; ++i)
    {
        engine_shaders[i] = {};
    }
    shaders_blob.reset();
}

D3D12_SHADER_BYTECODE get_engine_shader(engine_shader::id id)
{
    LASSERT(id < engine_shader::count);
    const compiled_shader_ptr shader = engine_shaders[id];
    LASSERT(shader && shader->size);
    return {&shader->byte_code, shader->size};
}

} // namespace lotus::graphics::d3d12::shaders