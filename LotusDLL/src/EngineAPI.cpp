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
// File Name: EngineAPI.cpp
// Date File Created: 08/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------

#ifndef EDITOR_INTERFACE
    #define EDITOR_INTERFACE extern "C" __declspec(dllexport)
#endif

#include <Lotus/Core/Common.h>
#include <Lotus/Core/Id.h>
#include <Lotus/Components/Entity.h>
#include <Lotus/Components/Transform.h>

using namespace lotus;

namespace
{

struct TransformDesc
{
    f32 position [ 3 ];
    f32 rotation [ 3 ];
    f32 scale [ 3 ];

    TransformInfo ToTransformData()
    {
        using namespace DirectX;
        TransformInfo data {};
        memcpy(&data.position [ 0 ], &position [ 0 ], sizeof(f32) * _countof(position));
        memcpy(&data.scale [ 0 ], &scale [ 0 ], sizeof(f32) * _countof(scale));

        vec3a rot { &rotation [ 0 ] };
        vec   quat { XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
        vec4a rotQuad {};
        XMStoreFloat4A(&rotQuad, quat);
        memcpy(&data.rotation [ 0 ], &rotQuad.x, sizeof(f32) * _countof(data.rotation));

        return data;
    }
};

struct EntityDesc
{
    TransformDesc transform;
};


Entity EntityFromId(id::id_type id) { return Entity { EntityId { id } }; }


} // namespace

EDITOR_INTERFACE id::id_type CreateEntity(EntityDesc* e)
{
    LASSERT(e);
    EntityDesc&   desc { *e };
    TransformInfo transform = desc.transform.ToTransformData();
    EntityInfo    entity    = { &transform };

    return CreateEntity(entity).GetId();
}

EDITOR_INTERFACE void RemoveEntity(id::id_type id)
{
    LASSERT(id::is_valid(id));
    RemoveEntity(EntityFromId(id));
}