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
// File Name: TestRenderer.cpp
// Date File Created: 10/30/2022
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "TestRenderer.h"

#include <filesystem>
#include <fstream>

#include "ShaderCompiler.h"

#include "Lotus/Platform/Platform.h"
#include "Lotus/Graphics/Renderer.h"
#include "Lotus/Graphics/D3D12/D3D12Core.h"

#include "Lotus/Content/ContentToEngine.h"

#include "Lotus/Components/Entity.h"
#include "Lotus/Components/Transform.h"
#include "Lotus/Components/Script.h"

#if TEST_RENDERER

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

    // Multithreading
    #define ENABLE_TEST_WORKERS 0
constexpr u32 num_threads     = 8;
bool          should_shutdown = false;
std::thread   workers[num_threads];

utl::vector<u8> buffer(1_MB, 0);

void buffer_test_worker()
{
    while (!should_shutdown)
    {
        auto* res = graphics::d3d12::d3dx::create_buffer(buffer.data(), (u32) buffer.size());
        graphics::d3d12::core::deferred_release(res);
    }
}


template<class Func, class... Args>
void init_test_workers(Func&& fn, Args&&... args)
{
    #if ENABLE_TEST_WORKERS
    should_shutdown = false;
    for (auto& w : workers)
    {
        w = std::thread(std::forward<Func>(fn), std::forward<Args>(args)...);
    }
    #endif
}

void join_test_workers()
{
    #if ENABLE_TEST_WORKERS
    should_shutdown = true;
    for (auto& w : workers)
    {
        w.join();
    }
    #endif
}

/////////////////////

struct camera_surface
{
    game_entity::entity      entity{};
    graphics::camera         camera{};
    graphics::render_surface surface{};
};

constexpr u32 num_windows = 4;

timer_lt timer;

camera_surface surfaces[num_windows];

id::id_type model_id = id::invalid_id;
id::id_type item_id  = id::invalid_id;

bool is_restarting = false;
bool resized       = false;

void destroy_camera_surface(camera_surface& surface);
bool test_initialize();
void test_shutdown();

id::id_type create_render_item(id::id_type entity_id);
void        destroy_render_item(id::id_type id);

void generate_lights();
void remove_lights();

LRESULT winproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    bool toggle_fullscreen = false;

    switch (msg)
    {
    case WM_DESTROY:
    {
        bool all = true;
        for (auto& s : surfaces)
        {
            if (s.surface.window.is_valid())
            {
                if (s.surface.window.is_closed())
                {
                    destroy_camera_surface(s);
                } else
                {
                    all = false;
                }
            }
        }

        if (all && !is_restarting)
        {
            PostQuitMessage(0);
            return 0;
        }
    }
    break;
    case WM_SIZE: resized = (wparam != SIZE_MINIMIZED); break;
    case WM_SYSCHAR:
        toggle_fullscreen = (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN));
        if (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN))
            break;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;
        } else if (wparam == VK_F11)
        {
            is_restarting = true;
            test_shutdown();
            test_initialize();
        }

    default: break;
    }

    if ((resized && GetAsyncKeyState(VK_LBUTTON) >= 0) || toggle_fullscreen)
    {
        platform::window win{ platform::window_id{ (id::id_type) GetWindowLongPtr(hwnd, GWLP_USERDATA) } };
        for (u32 i = 0; i < _countof(surfaces); ++i)
        {
            if (win.get_id() == surfaces[i].surface.window.get_id())
            {
                if (toggle_fullscreen)
                {
                    win.set_fullscreen(!win.is_fullscreen());
                    return 0;
                } else
                {
                    surfaces[i].surface.surface.resize(win.width(), win.height());
                    surfaces[i].camera.aspect_ratio((f32) win.width() / (f32) win.height());
                    resized = false;
                }
                break;
            }
        }
    }

    return DefWindowProc(hwnd, msg, wparam, lparam);
}

game_entity::entity create_one_entity(vec3 position, vec3 rotation, const char* script_name)
{
    transform::create_info transform_info{};
    vec                    quat = math::quat_rotation_roll_pitch_yaw_from_vec(math::load_float3(&rotation));
    vec4a                  rot_quat;
    math::store_float4a(&rot_quat, quat);
    memcpy(&transform_info.rotation[0], &rot_quat.x, sizeof(transform_info.rotation));
    memcpy(&transform_info.position[0], &position.x, sizeof(transform_info.position));

    script::create_info script_info{};
    if (script_name)
    {
        script_info.script_creator = script::detail::get_script_creator(string_hash()(script_name));
        assert(script_info.script_creator);
    }


    game_entity::create_info entity_info{};
    entity_info.transform = &transform_info;
    entity_info.script    = &script_info;
    game_entity::entity ent(game_entity::create(entity_info));
    assert(ent.is_valid());
    return ent;
}

void remove_game_entity(game_entity::entity_id id)
{
    game_entity::remove(id);
}

void create_camera_surface(camera_surface& surface, platform::window_create_info info)
{
    surface.surface.window  = platform::create_window(&info);
    surface.surface.surface = graphics::create_surface(surface.surface.window);
    surface.entity          = create_one_entity({ 0.0f, 1.0f, 3.0f }, { 0.0f, 3.14f, 0.0f }, nullptr);
    surface.camera          = graphics::create_camera(graphics::perspective_camera_init_info{ surface.entity.get_id() });
    surface.camera.aspect_ratio((f32) surface.surface.window.width() / (f32) surface.surface.window.height());
}

void destroy_camera_surface(camera_surface& surface)
{
    camera_surface temp{ surface };
    surface = {};
    if (temp.surface.surface.is_valid())
        graphics::remove_surface(temp.surface.surface.get_id());
    if (temp.surface.window.is_valid())
        platform::remove_window(temp.surface.window.get_id());
    if (temp.camera.is_valid())
        graphics::remove_camera(temp.camera.get_id());
    if (temp.entity.is_valid())
        game_entity::remove(temp.entity.get_id());
}


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

bool test_initialize()
{
    while (!compile_shaders())
    {
        if (MessageBox(nullptr, L"Failed to compile engine shaders", L"Shader Compilation Error", MB_RETRYCANCEL) != IDRETRY)
            return false;
    }

    if (!graphics::initialize(graphics::graphics_platform::d3d12))
        return false;

    platform::window_create_info info[num_windows]{
        {&winproc, nullptr, L"Test Window 1", 100, 100, 400, 800},
        {&winproc, nullptr, L"Test Window 2", 200, 200, 400, 400},
        {&winproc, nullptr, L"Test Window 3", 300, 300, 800, 400},
        {&winproc, nullptr, L"Test Window 4", 400, 400, 800, 600},
    };


    static_assert(_countof(info) == _countof(surfaces));

    for (u32 i = 0; i < num_windows; ++i)
    {
        create_camera_surface(surfaces[i], info[i]);
    }

    scope<u8[]> model;
    u64         size = 0;
    if (!read_file(R"(..\..\Tests\model.model)", model, size))
        return false;
    model_id = content::create_resource(model.get(), content::asset_type::mesh);
    if (!id::is_valid(model_id))
        return false;

    init_test_workers(buffer_test_worker);


    item_id = create_render_item(create_one_entity({}, {}, "rotator_script").get_id());
    generate_lights();
    is_restarting = false;
    return true;
}

void test_shutdown()
{
    remove_lights();
    destroy_render_item(item_id);
    join_test_workers();


    if (id::is_valid(model_id))
    {
        content::destroy_resource(model_id, content::asset_type::mesh);
    }

    for (auto& s : surfaces)
    {
        destroy_camera_surface(s);
    }

    graphics::shutdown();
}

bool EngineTest::Init()
{
    return test_initialize();
}
void EngineTest::Run()
{
    timer.begin();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    script::update_all(timer.delta_average());
    for (u32 i = 0; i < num_windows; ++i)
    {
        if (surfaces[i].surface.surface.is_valid())
        {
            f32 threshold{ 10 };

            graphics::frame_info info{};
            info.render_item_ids    = &item_id;
            info.render_item_count  = 1;
            info.thresholds         = &threshold;
            info.light_set_key      = 0;
            info.average_frame_time = timer.delta_average();
            info.cam_id             = surfaces[i].camera.get_id();

            surfaces[i].surface.surface.render(info);
        }
    }
    timer.end();
}

void EngineTest::Shutdown()
{
    test_shutdown();
}

#endif