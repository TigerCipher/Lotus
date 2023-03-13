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
#include "Components/Entity.h"
#include "Graphics/Renderer.h"

#include "ShaderCompiler.h"

using namespace lotus;

bool read_file(std::filesystem::path path, scope<u8[]>& data, u64& size);

namespace
{
id::id_type model_id = id::invalid_id;
id::id_type vs_id    = id::invalid_id;
id::id_type ps_id    = id::invalid_id;
id::id_type mtl_id   = id::invalid_id;

std::unordered_map<id::id_type, id::id_type> render_item_map;

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
    shader_file_info info{};
    info.file_name = "TestShader.hlsl";
    info.function  = "TestShaderVS";
    info.type      = shader_type::vertex;

    const char* path          = "..\\..\\Tests\\";
    auto        vertex_shader = compile_shader(info, path);
    LASSERT(vertex_shader.get());

    info.function = "TestShaderPS";
    info.type     = shader_type::pixel;

    auto pixel_shader = compile_shader(info, path);
    LASSERT(pixel_shader.get());

    vs_id = content::add_shader(vertex_shader.get());
    ps_id = content::add_shader(pixel_shader.get());
}

void create_material()
{
    graphics::material_init_info info{};
    info.shader_ids[graphics::shader_type::vertex] = vs_id;
    info.shader_ids[graphics::shader_type::pixel]  = ps_id;
    info.type                                      = graphics::material_type::opaque;

    mtl_id = content::create_resource(&info, content::asset_type::material);
}

} // anonymous namespace


id::id_type create_render_item(id::id_type entity_id)
{
    // load model
    auto _1 = std::thread([] { load_model(); });

    // load material
    // - texture
    // - shaders
    auto _2 = std::thread([] { load_shaders(); });

    _1.join();
    _2.join();

    // add render item and its materials
    //create_material();

    id::id_type item_id      = 0; // temp
    render_item_map[item_id] = entity_id;

    return item_id;
}

void destroy_render_item(id::id_type id)
{
    // remove render item, entity
    if (id::is_valid(id))
    {
        auto pair = render_item_map.find(id);
        if (pair != render_item_map.end())
        {
            game_entity::remove(game_entity::entity_id{ pair->second });
        }
    }
    // remove material

    if (id::is_valid(mtl_id))
    {
        content::destroy_resource(mtl_id, content::asset_type::material);
    }

    // remove shaders, textures

    if (id::is_valid(vs_id))
    {
        content::remove_shader(vs_id);
    }

    if (id::is_valid(ps_id))
    {
        content::remove_shader(ps_id);
    }

    // remove model

    if (id::is_valid(model_id))
    {
        content::destroy_resource(model_id, content::asset_type::mesh);
    }
}