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
#include "pch.h"
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
    enum ComponentType
    {
        TRANSFORM,
        SCRIPT,
        COUNT
    };

    using comp_type = bool (*)(const byte*&, entity::create_info&);

    utl::vector<entity::Entity> entities;
    transform::create_info      transformInfo;
    script::create_info         scriptInfo;

    bool read_transform(const byte*& data, entity::create_info& info)
    {
        LASSERT(!info.transform);
        using namespace DirectX;

        f32 rotation [ 3 ];

        memcpy(&transformInfo.position [ 0 ], data, sizeof(transformInfo.position));
        data += sizeof(transformInfo.position);

        memcpy(&rotation [ 0 ], data, sizeof(rotation));
        data += sizeof(rotation);

        memcpy(&transformInfo.scale [ 0 ], data, sizeof(transformInfo.scale));
        data += sizeof(transformInfo.scale);

        vec3a rot { &rotation [ 0 ] };
        vec   quat { XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
        vec4a rotQuad {};
        XMStoreFloat4A(&rotQuad, quat);
        memcpy(&transformInfo.rotation [ 0 ], &rotQuad.x, sizeof(transformInfo.rotation));

        info.transform = &transformInfo;
        return true;
    }

    bool read_script(const byte*& data, entity::create_info& info)
    {
        LASSERT(!info.script);
        const u32 nameLength = *data;
        data += sizeof(u32);

        if (!nameLength) return false;

        LASSERT(nameLength < 256);
        char scriptName [ 256 ];
        memcpy(&scriptName [ 0 ], data, nameLength);
        data += nameLength;

        scriptName [ nameLength ] = 0; // Null terminate the script name

        scriptInfo.scriptCreator = script::detail::get_script_creator(string_hash()(scriptName));

        info.script = &scriptInfo;
        return scriptInfo.scriptCreator != nullptr;
    }

    using comp_reader = bool (*)(const byte*&, entity::create_info&);

    comp_reader compReaders [] { read_transform, read_script };

    static_assert(_countof(compReaders) == ComponentType::COUNT);

} // namespace

bool load_game()
{
    wchar_t path [ MAX_PATH ];
    const u32 length = GetModuleFileName(nullptr, &path [ 0 ], MAX_PATH);
    if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER) return false;

    std::filesystem::path p = path;
    SetCurrentDirectory(p.parent_path().wstring().c_str());

    std::ifstream     game("game.bin", std::ios::in | std::ios::binary);
    utl::vector<byte> buffer(std::istreambuf_iterator<char>(game), {});

    LASSERT(buffer.size());

    const byte*   at     = buffer.data();
    constexpr u32 size32 = sizeof(u32);

    const u32 numEnts = *at;
    at += size32;

    if (!numEnts) return false;

    for (u32 i = 0; i < numEnts; ++i)
    {
        entity::create_info info;
        const u32           entityType = *at;
        at += size32;
        const u32 compCount = *at;
        at += size32;

        if (!compCount) return false;

        for (u32 compIndex = 0; compIndex < compCount; ++compIndex)
        {
            const u32 compType = *at;
            at += size32;
            LASSERT(compIndex == 0 || compIndex == 1);
            LASSERT(compType < ComponentType::COUNT);

            if (!compReaders [ compType ](at, info)) return false;
        }

        LASSERT(info.transform);
        entity::Entity ent(entity::create(info));
        if (!ent.IsValid()) return false;
        entities.emplace_back(ent);
    }

    LASSERT(at == buffer.data() + buffer.size());
    return true;
}

void unload_game()
{
    for (auto ent : entities) { entity::remove(ent.GetId()); }
}
} // namespace lotus::content
