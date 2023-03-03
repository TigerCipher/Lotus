//  ------------------------------------------------------------------------------
// 
//  Lotus
//     Copyright 2023 Matthew Rogers
// 
//     Licensed under the Apache License, Version 2.0 (the "License");
//     you may not use this file except in compliance with the License.
//     You may obtain a copy of the License at
// 
//         http://www.apache.org/licenses/LICENSE-2.0
// 
//     Unless required by applicable law or agreed to in writing, software
//     distributed under the License is distributed on an "AS IS" BASIS,
//     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//     See the License for the specific language governing permissions and
//     limitations under the License.
// 
//  File Name: RenderItem.cpp
//  Date File Created: 03/03/2023
//  Author: Matt
// 
//  ------------------------------------------------------------------------------

#include <filesystem>
#include <Lotus/Common.h>

#include "Content/ContentToEngine.h"

#include "ShaderCompiler.h"

using namespace lotus;

bool read_file(std::filesystem::path path, scope<u8[]>& data, u64& size);

namespace
{
    id::id_type model_id = id::invalid_id;
    id::id_type vs_id = id::invalid_id;
    id::id_type ps_id = id::invalid_id;

void load_model()
{
    scope<u8[]> model;
    u64         size = 0;
    read_file(R"(..\..\Tests\model.model)", model, size);
    model_id = content::create_resource(model.get(), content::asset_type::mesh);
    LASSERT(id::is_valid(model_id));
}

void load_shaders()
{
    
}

} // anonymous namespace


id::id_type create_render_item(id::id_type entity_id)
{
    // load model
    auto _1 = std::thread([]{load_model();});

    // load material
    // - texture
    // - shaders
    auto _2 = std::thread([]{load_shaders();});

    _1.join();
    _2.join();

    // add render item and its materials

    return id::invalid_id;
}

void        destroy_render_item(id::id_type id)
{
    // remove render item, entity
    // remove material
    // remove shaders, textures
    // remove model
}