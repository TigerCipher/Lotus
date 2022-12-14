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

#include "../Components/Entity.h"
#include "../Components/Transform.h"
#include "../Components/Script.h"

#include <fstream>
#include <filesystem>

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace lotus::content
{

namespace
{
enum component_type
{
    TRANSFORM,
    SCRIPT,
    COUNT
};

using comp_type = bool (*)(const byte*&, entity::create_info&);

utl::vector<entity::entity> entities;
transform::create_info      transform_info;
script::create_info         script_info;

bool read_transform(const byte*& data, entity::create_info& info)
{
    LASSERT(!info.transform);
    using namespace DirectX;

    f32 rotation[3];

    memcpy(&transform_info.position[0], data, sizeof(transform_info.position));
    data += sizeof(transform_info.position);

    memcpy(&rotation[0], data, sizeof(rotation));
    data += sizeof(rotation);

    memcpy(&transform_info.scale[0], data, sizeof(transform_info.scale));
    data += sizeof(transform_info.scale);

    vec3a rot{&rotation[0]};
    // TODO: Move XM functions to math util
    vec   quat{XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot))};
    vec4a rotQuad{};
    XMStoreFloat4A(&rotQuad, quat);
    memcpy(&transform_info.rotation[0], &rotQuad.x, sizeof(transform_info.rotation));

    info.transform = &transform_info;
    return true;
}

bool read_script(const byte*& data, entity::create_info& info)
{
    LASSERT(!info.script);
    const u32 name_length = *data;
    data += sizeof(u32);

    if (!name_length)
        return false;

    LASSERT(name_length < 256);
    char scriptName[256];
    memcpy(&scriptName[0], data, name_length);
    data += name_length;

    scriptName[name_length] = 0; // Null terminate the script name

    script_info.script_creator = script::detail::get_script_creator(string_hash()(scriptName));

    info.script = &script_info;
    return script_info.script_creator != nullptr;
}

using comp_reader = bool (*)(const byte*&, entity::create_info&);

comp_reader comp_readers[]{read_transform, read_script};

static_assert(_countof(comp_readers) == component_type::COUNT);

} // namespace

bool load_game()
{
    wchar_t   path[MAX_PATH];
    const u32 length = GetModuleFileName(nullptr, &path[0], MAX_PATH);
    if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        return false;

    std::filesystem::path p = path;
    SetCurrentDirectory(p.parent_path().wstring().c_str());

    std::ifstream     game("game.bin", std::ios::in | std::ios::binary);
    utl::vector<byte> buffer(std::istreambuf_iterator<char>(game), {});

    LASSERT(buffer.size());

    const byte*   at     = buffer.data();
    constexpr u32 size32 = sizeof(u32);

    const u32 num_ents = *at;
    at += size32;

    if (!num_ents)
        return false;

    for (u32 i = 0; i < num_ents; ++i)
    {
        entity::create_info info;
        const u32           entity_type = *at; // TODO
        at += size32;
        const u32 comp_count = *at;
        at += size32;

        if (!comp_count)
            return false;

        for (u32 comp_index = 0; comp_index < comp_count; ++comp_index)
        {
            const u32 comp_type = *at;
            at += size32;
            LASSERT(comp_index == 0 || comp_index == 1);
            LASSERT(comp_type < component_type::COUNT);

            if (!comp_readers[comp_type](at, info))
                return false;
        }

        LASSERT(info.transform);
        entity::entity ent(entity::create(info));
        if (!ent.is_valid())
            return false;
        entities.emplace_back(ent);
    }

    LASSERT(at == buffer.data() + buffer.size());
    return true;
}

void unload_game()
{
    for (auto ent : entities)
    {
        entity::remove(ent.get_id());
    }
}
} // namespace lotus::content
