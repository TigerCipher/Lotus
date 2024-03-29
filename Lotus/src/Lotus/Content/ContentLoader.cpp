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
// File Name: ContentLoader.cpp
// Date File Created: 8/26/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "ContentLoader.h"

#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Components/Script.h"
#include "Graphics/Renderer.h"

#include <fstream>
#include <filesystem>


namespace lotus::content
{

namespace
{
enum component_type
{
    transform = 0,
    script,


    count
};


utl::vector<game_entity::entity> entities;
transform::create_info      transform_info;
script::create_info         script_info;

bool read_transform(const u8*& data, game_entity::create_info& info)
{
    assert(!info.transform);

    f32 rotation[3];

    memcpy(&transform_info.position[0], data, sizeof(transform_info.position));
    data += sizeof(transform_info.position);

    memcpy(&rotation[0], data, sizeof(rotation));
    data += sizeof(rotation);

    memcpy(&transform_info.scale[0], data, sizeof(transform_info.scale));
    data += sizeof(transform_info.scale);

    vec3a rot{ &rotation[0] };
    vec   quat{ math::quat_rotation_roll_pitch_yaw_from_vec(math::load_float3a(&rot)) };
    vec4a rotQuad{};
    math::store_float4a(&rotQuad, quat);
    memcpy(&transform_info.rotation[0], &rotQuad.x, sizeof(transform_info.rotation));

    info.transform = &transform_info;
    return true;
}

bool read_script(const u8*& data, game_entity::create_info& info)
{
    assert(!info.script);
    const u32 name_length = *data;
    data += sizeof(u32);

    if (!name_length)
        return false;

    assert(name_length < 256);
    char script_name[256];
    memcpy(&script_name[0], data, name_length);
    data += name_length;

    script_name[name_length] = 0; // Null terminate the script name

    script_info.script_creator = script::detail::get_script_creator(string_hash()(script_name));

    info.script = &script_info;
    return script_info.script_creator != nullptr;
}

using comp_reader = bool (*)(const u8*&, game_entity::create_info&);

comp_reader comp_readers[]{ read_transform, read_script };

static_assert(_countof(comp_readers) == component_type::count);

bool read_file(std::filesystem::path path, scope<u8[]>& data, u64& size)
{
    if (!std::filesystem::exists(path))
        return false;
    size = std::filesystem::file_size(path);
    assert(size);
    if (!size)
        return false;
    data = create_scope<u8[]>(size);
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file || !file.read((char*) data.get(), size))
    {
        file.close();
        return false;
    }

    file.close();
    return true;
}
} // namespace

bool load_game()
{
    scope<u8[]> game_data{};
    u64         size{ 0 };
    if (!read_file("game.bin", game_data, size))
    {
        return false;
    }
    assert(game_data.get());
    const u8*     at     = game_data.get();
    constexpr u32 size32 = sizeof(u32);

    const u32 num_ents = *at;
    at += size32;

    if (!num_ents)
        return false;

    for (u32 i = 0; i < num_ents; ++i)
    {
        game_entity::create_info info;
        //         const u32           entity_type = *at; // TODO
        at += size32;
        const u32 comp_count = *at;
        at += size32;

        if (!comp_count)
            return false;

        for (u32 comp_index = 0; comp_index < comp_count; ++comp_index)
        {
            const u32 comp_type = *at;
            at += size32;
            assert(comp_index == 0 || comp_index == 1);
            assert(comp_type < component_type::count);

            if (!comp_readers[comp_type](at, info))
                return false;
        }

        assert(info.transform);
        game_entity::entity ent(game_entity::create(info));
        if (!ent.is_valid())
            return false;
        entities.emplace_back(ent);
    }

    assert(at == game_data.get() + size);
    return true;
}

void unload_game()
{
    for (auto ent : entities)
    {
        game_entity::remove(ent.get_id());
    }
}

bool load_engine_shaders(scope<u8[]>& shaders_blob, u64& size)
{
    const auto path = graphics::get_engine_shaders_path();
    return read_file(path, shaders_blob, size);
}

} // namespace lotus::content
