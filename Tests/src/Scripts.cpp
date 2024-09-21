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
// File Name: Scripts
// Date File Created: 08/17/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#include <Lotus/Components/Entity.h>
#include <Lotus/Components/Transform.h>
#include <Lotus/Components/Script.h>
#include <Lotus/API/Input.h>

using namespace lotus;

class rotator_script : public script::entity_script
{
public:
    constexpr explicit rotator_script(game_entity::entity entity) : script::entity_script{ entity } {}

    void on_start() override {}
    void update(f32 delta) override
    {
        m_angle += 0.25f * delta * math::two_pi;
        if (m_angle > math::two_pi)
        {
            m_angle -= math::two_pi;
        }
        vec3a rot{ 0.0f, m_angle, 0.0f };
        vec   quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };
        vec4  rot_quat{};
        DirectX::XMStoreFloat4(&rot_quat, quat);
        set_rotation(rot_quat);
    }

private:
    f32 m_angle{};
};
LOTUS_REGISTER_SCRIPT(rotator_script);

class fan_script : public script::entity_script
{
public:
    constexpr explicit fan_script(game_entity::entity entity) : script::entity_script{ entity } {}

    void on_start() override {}
    void update(f32 delta) override
    {
        m_angle -= 1.0f * delta * math::two_pi;
        if (m_angle > math::two_pi)
        {
            m_angle += math::two_pi;
        }
        vec3a rot{ m_angle, 0.0f, 0.0f };
        vec   quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };
        vec4  rot_quat{};
        DirectX::XMStoreFloat4(&rot_quat, quat);
        set_rotation(rot_quat);
    }

private:
    f32 m_angle{};
};
LOTUS_REGISTER_SCRIPT(fan_script);


class wibbly_wobbly_script : public script::entity_script
{
public:
    constexpr explicit wibbly_wobbly_script(game_entity::entity entity) : script::entity_script{ entity } {}

    void on_start() override {}
    void update(float dt) override
    {
        _angle -= 0.01f * dt * math::two_pi;
        if (_angle > math::two_pi)
            _angle += math::two_pi;
        f32 x{ _angle * 2.f - math::pi };

        const f32 s1{ 0.05f * std::sin(x) * std::sin(std::sin(x / 1.62f) + std::sin(1.62f * x) + std::sin(3.24f * x)) };
        x = _angle;
        const f32 s2{ 0.05f * std::sin(x) * std::sin(std::sin(x / 1.62f) + std::sin(1.62f * x) + std::sin(3.24f * x)) };

        vec3a             rot{ s1, 0.f, s2 };
        DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };
        vec4              rot_quat{};
        DirectX::XMStoreFloat4(&rot_quat, quat);
        set_rotation(rot_quat);
        vec3 pos{ position() };
        pos.y = 1.3f + 0.2f * std::sin(x) * std::sin(std::sin(x / 1.62f) + std::sin(1.62f * x) + std::sin(3.24f * x));
        set_position(pos);
    }

private:
    f32 _angle{ 0.f };
};

LOTUS_REGISTER_SCRIPT(wibbly_wobbly_script);


class camera_script : public script::entity_script
{
public:
    explicit camera_script(game_entity::entity entity) : script::entity_script{ entity }
    {
        m_input.add_handler(input::input_source::mouse, this, &camera_script::mouse_move);
        vec3 dir = orientation();
        f32 theta = DirectX::XMScalarACos(dir.y);
        f32 phi = std::atan2(-dir.z, dir.x);
        vec3 rot = {theta - math::half_pi, phi + math::half_pi, 0.0f};
        m_spherical = DirectX::XMLoadFloat3(&rot);
    }

    void on_start() override {}
    void update(f32 delta) override
    {
    }

private:

    void mouse_move(input::input_source::type type, input::input_code::code code, const input::input_value& mouse_pos)
    {
        if(code == input::input_code::mouse_position)
        {
            input::input_value value;
            input::get(input::input_source::mouse, input::input_code::mouse_left, value);
            if(value.current.z == 0.0f) return;

            const f32 scale = 0.005f;
            const f32 dx = (mouse_pos.current.x - mouse_pos.previous.x) * scale;
            const f32 dy = (mouse_pos.current.y - mouse_pos.previous.y) * scale;

            vec3 spherical;
            DirectX::XMStoreFloat3(&spherical, m_spherical);
            spherical.x += dy;
            spherical.y -= dx;
            spherical.x = math::clamp(spherical.x, 0.0001f - math::half_pi, math::half_pi - 0.0001f);
            m_spherical = XMLoadFloat3(&spherical);
            vec quat = DirectX::XMQuaternionRotationRollPitchYawFromVector(m_spherical);
            vec4 rot_quat;
            XMStoreFloat4(&rot_quat, quat);
            set_rotation(rot_quat);
        }
    }
    
    input::input_system<camera_script> m_input;
    vec m_spherical;
};
LOTUS_REGISTER_SCRIPT(camera_script);