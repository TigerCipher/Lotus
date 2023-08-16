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
// File Name: Lights
// Date File Created: 08/16/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#include <Lotus/API/GameEntity.h>
#include <Lotus/API/Light.h>
#include <Lotus/API/TransformComponent.h>
#include <Lotus/Graphics/Renderer.h>

using namespace lotus;

game_entity::entity create_one_entity(vec3 position, vec3 rotation, const char* script_name);
void remove_game_entity(game_entity::entity_id id);

namespace
{
const u64 left_set{0};
const u64 right_set{0};

utl::vector<graphics::light> lights;

constexpr vec3 rgb_to_color(u8 r, u8 g, u8 b)
{
    return { r / 255.0f, g / 255.0f, b / 255.0f};
}

} // anonymous namespace

void generate_lights()
{
    // LEFT_SET
    graphics::light_init_info info{};
    info.entity_id     = create_one_entity({}, { 0, 0, 0 }, nullptr).get_id();
    info.type          = graphics::light::directional;
    info.light_set_key = left_set;
    info.intensity     = 1.f;
    info.color         = rgb_to_color(174, 174, 174);

    lights.emplace_back(graphics::create_light(info));

    info.entity_id = create_one_entity({}, { math::pi * 0.5f, 0, 0 }, nullptr).get_id();
    info.color     = rgb_to_color(17, 27, 48);
    lights.emplace_back(graphics::create_light(info));

    info.entity_id = create_one_entity({}, { -math::pi * 0.5f, 0, 0 }, nullptr).get_id();
    info.color     = rgb_to_color(63, 47, 30);
    lights.emplace_back(graphics::create_light(info));

    // RIGHT_SET
    info.entity_id     = create_one_entity({}, { 0, 0, 0 }, nullptr).get_id();
    info.light_set_key = right_set;
    info.color         = rgb_to_color(150, 100, 200);
    lights.emplace_back(graphics::create_light(info));

    info.entity_id = create_one_entity({}, { math::pi * 0.5f, 0, 0 }, nullptr).get_id();
    info.color     = rgb_to_color(17, 27, 48);
    lights.emplace_back(graphics::create_light(info));

    info.entity_id = create_one_entity({}, { -math::pi * 0.5f, 0, 0 }, nullptr).get_id();
    info.color     = rgb_to_color(63, 47, 30);
    lights.emplace_back(graphics::create_light(info));
}

void remove_lights()
{
    for (auto& light : lights)
    {
        const game_entity::entity_id id{ light.entity_id() };
        graphics::remove_light(light.get_id(), light.light_set_key());
        remove_game_entity(id);
    }

    lights.clear();
}