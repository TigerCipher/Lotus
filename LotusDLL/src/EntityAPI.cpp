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
// File Name: EntityAPI.cpp
// Date File Created: 8/24/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "Common.h"
#include <Lotus/Core/Common.h>
#include <Lotus/Core/Id.h>
#include <Lotus/Components/Entity.h>
#include <Lotus/Components/Transform.h>
#include <Lotus/Components/Script.h>

using namespace lotus;

namespace
{

struct transform_component
{
    f32 position [ 3 ];
    f32 rotation [ 3 ];
    f32 scale [ 3 ];

    transform::create_info ToTransformData()
    {
        using namespace DirectX;
        transform::create_info data {};
        memcpy(&data.position [ 0 ], &position [ 0 ], sizeof(position));
        memcpy(&data.scale [ 0 ], &scale [ 0 ], sizeof(scale));

        vec3a rot { &rotation [ 0 ] };
        vec   quat { XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
        vec4a rotQuad {};
        XMStoreFloat4A(&rotQuad, quat);
        memcpy(&data.rotation [ 0 ], &rotQuad.x, sizeof(rotation));

        return data;
    }
};

struct script_component
{
    script::detail::script_creator scriptCreator;

    script::create_info to_create_info()
    {
        script::create_info info;
        info.scriptCreator = scriptCreator;
        return info;
    }
};

struct EntityDesc
{
    transform_component transform;
    script_component    script;
};


entity::Entity EntityFromId(id::id_type id) { return entity::Entity { entity::entity_id { id } }; }


} // namespace

EDITOR_INTERFACE id::id_type CreateEntity(EntityDesc* e)
{
    LASSERT(e);
    EntityDesc&            desc { *e };
    transform::create_info transform = desc.transform.ToTransformData();
    script::create_info    scriptInfo = desc.script.to_create_info();
    entity::create_info    entity    = { &transform, &scriptInfo };

    return entity::create(entity).GetId();
}

EDITOR_INTERFACE void RemoveEntity(id::id_type id)
{
    LASSERT(id::is_valid(id));
    entity::remove(entity::entity_id { id });
}
