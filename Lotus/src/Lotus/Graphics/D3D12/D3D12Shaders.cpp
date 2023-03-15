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
#include "Content/ContentToEngine.h"

namespace lotus::graphics::d3d12::shaders
{

namespace
{
content::compiled_shader_ptr engine_shaders[engine_shader::count]{};

// memory containing all compiled shaders in format of size->bytecode->size->bytecode...
scope<byte[]> engine_shaders_blob{};

bool load_engine_shaders()
{
    assert(!engine_shaders_blob);
    u64 size = 0;

    bool result = content::load_engine_shaders(engine_shaders_blob, size);
    assert(engine_shaders_blob && size);

    u64 offset = 0;
    u32 idx    = 0;
    while (offset < size && result)
    {
        assert(idx < engine_shader::count);
        content::compiled_shader_ptr& shader = engine_shaders[idx];
        assert(!shader);
        result &= idx < engine_shader::count && !shader;
        if (!result)
            break;
        shader = reinterpret_cast<const content::compiled_shader_ptr>(&engine_shaders_blob[offset]);
        offset += sizeof(u64) + content::compiled_shader::hash_length + shader->byte_code_size();
        ++idx;
    }

    assert(offset == size && idx == engine_shader::count);

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
    engine_shaders_blob.reset();
}

D3D12_SHADER_BYTECODE get_engine_shader(engine_shader::id id)
{
    assert(id < engine_shader::count);
    const content::compiled_shader_ptr& shader = engine_shaders[id];
    assert(shader && shader->byte_code_size());
    return { shader->byte_code(), shader->byte_code_size() };
}

} // namespace lotus::graphics::d3d12::shaders