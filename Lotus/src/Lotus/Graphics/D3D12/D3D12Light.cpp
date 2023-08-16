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
// File Name: D3D12Light
// Date File Created: 08/16/2023
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "D3D12Light.h"
#include "API/GameEntity.h"
#include "Shaders/SharedTypes.h"

namespace lotus::graphics::d3d12::light
{

namespace
{

struct light_owner
{
    game_entity::entity_id entity_id{ id::invalid_id };
    u32                    data_index;
    graphics::light::type  type;
    bool                   is_enabled;
};

#if USE_STL_VECTOR
    #define CONSTEXPR
#else
    #define CONSTEXPR constexpr
#endif

class light_set
{
public:
    constexpr graphics::light add(const light_init_info& info)
    {
        if (info.type == graphics::light::directional)
        {
            u32 index{ invalid_id_u32 };
            for (u32 i = 0; i < m_non_cullable_owners.size(); ++i)
            {
                if (!id::is_valid(m_non_cullable_owners[i]))
                {
                    index = i;
                    break;
                }
            }

            if (index == invalid_id_u32)
            {
                index = (u32) m_non_cullable_owners.size();
                m_non_cullable_owners.emplace_back();
                m_non_cullable_lights.emplace_back();
            }

            hlsl::DirectionalLightParameters& params{ m_non_cullable_lights[index] };
            params.Color     = info.color;
            params.Intensity = info.intensity;

            light_owner    owner{ game_entity::entity_id{ info.entity_id }, index, info.type, info.is_enabled };
            const light_id id{ m_owners.add(owner) };
            m_non_cullable_owners[index] = id;

            return graphics::light{ id, info.light_set_key };
        }

        // TODO: Cullable lights
        return {};
    }

    constexpr void remove(light_id id)
    {
        enable(id, false);

        const light_owner& owner{ m_owners[id] };

        if (owner.type == graphics::light::directional)
        {
            m_non_cullable_owners[owner.data_index] = light_id{ id::invalid_id };
        } else
        {
            // TODO: Cullable lights
        }

        m_owners.remove(id);
    }

    constexpr void enable(light_id id, bool is_enabled)
    {
        m_owners[id].is_enabled = is_enabled;

        if (m_owners[id].type == graphics::light::directional)
        {
            return;
        }
        // TODO: Cullable lights
    }

    constexpr void intensity(light_id id, f32 intensity)
    {
        if (intensity < 0.0f)
            intensity = 0.0f;

        const light_owner& owner{ m_owners[id] };
        const u32          index{ owner.data_index };

        if (owner.type == graphics::light::directional)
        {
            assert(index < m_non_cullable_lights.size());
            m_non_cullable_lights[index].Intensity = intensity;
        } else
        {
            // TODO: Cullable lights
        }
    }

    constexpr void color(light_id id, vec3 color)
    {
        assert(color.x <= 1.0f && color.y <= 1.0f && color.z <= 1.0f);
        assert(color.x >= 0.0f && color.y >= 0.0f && color.z >= 0.0f);

        const light_owner& owner{ m_owners[id] };
        const u32          index{ owner.data_index };

        if (owner.type == graphics::light::directional)
        {
            assert(index < m_non_cullable_lights.size());
            m_non_cullable_lights[index].Color = color;
        } else
        {
            // TODO: Cullable lights
        }
    }

    constexpr bool is_enabled(light_id id) const { return m_owners[id].is_enabled; }

    constexpr f32 intensity(light_id id) const
    {
        const light_owner& owner{ m_owners[id] };
        const u32          index{ owner.data_index };

        if (owner.type == graphics::light::directional)
        {
            assert(index < m_non_cullable_lights.size());
            return m_non_cullable_lights[index].Intensity;
        }
        // TODO: Cullable lights
        return 0.0f;
    }

    constexpr vec3 color(light_id id)
    {
        const light_owner& owner{ m_owners[id] };
        const u32          index{ owner.data_index };

        if (owner.type == graphics::light::directional)
        {
            assert(index < m_non_cullable_lights.size());
            return m_non_cullable_lights[index].Color;
        }
        // TODO: Cullable lights
        return {};
    }

    constexpr graphics::light::type type(light_id id) const { return m_owners[id].type; }
    constexpr id::id_type           entity_id(light_id id) const { return m_owners[id].entity_id; }

    // Number of enabled directional lights
    CONSTEXPR u32 non_cullable_light_count() const
    {
        u32 count{ 0 };
        for (const auto& id : m_non_cullable_owners)
        {
            if (id::is_valid(id) && m_owners[id].is_enabled)
                ++count;
        }

        return count;
    }

    CONSTEXPR void non_cullable_lights(hlsl::DirectionalLightParameters* const lights, [[maybe_unused]] u32 buffer_size)
    {
        assert(buffer_size == math::align_size_up<D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT>(
                                  non_cullable_light_count() * sizeof(hlsl::DirectionalLightParameters)));
        const u32 count{ (u32) m_non_cullable_owners.size() };
        u32       index{ 0 };
        for (u32 i = 0; i < count; ++i)
        {
            if (!id::is_valid(m_non_cullable_owners[i]))
                continue;

            const light_owner& owner{ m_owners[m_non_cullable_owners[i]] };
            if (owner.is_enabled)
            {
                assert(m_owners[m_non_cullable_owners[i]].data_index == i);
                lights[index] = m_non_cullable_lights[i];
                ++index;
            }
        }
    }

private:
    utl::free_list<light_owner>                   m_owners;
    utl::vector<light_id>                         m_non_cullable_owners;
    utl::vector<hlsl::DirectionalLightParameters> m_non_cullable_lights;
};

#undef CONSTEXPR

std::unordered_map<u64, light_set> light_sets;

constexpr void set_is_enabled(light_set& set, light_id id, const void* const data, [[maybe_unused]] u32 size)
{
    bool is_enabled{ *(bool*) data };
    assert(sizeof(is_enabled) == size);
    set.enable(id, is_enabled);
}

constexpr void set_intensity(light_set& set, light_id id, const void* const data, [[maybe_unused]] u32 size)
{
    f32 intensity{ *(f32*) data };
    assert(sizeof(intensity) == size);
    set.intensity(id, intensity);
}

constexpr void set_color(light_set& set, light_id id, const void* const data, [[maybe_unused]] u32 size)
{
    vec3 color{ *(vec3*) data };
    assert(sizeof(color) == size);
    set.color(id, color);
}

constexpr void get_is_enabled(light_set& set, light_id id, void* const data, [[maybe_unused]] u32 size)
{
    bool* const is_enabled{ (bool* const) data };
    assert(sizeof(bool) == size);
    *is_enabled = set.is_enabled(id);
}

constexpr void get_intensity(light_set& set, light_id id, void* const data, [[maybe_unused]] u32 size)
{
    f32* const intensity{ (f32* const) data };
    assert(sizeof(f32) == size);
    *intensity = set.intensity(id);
}

constexpr void get_color(light_set& set, light_id id, void* const data, [[maybe_unused]] u32 size)
{
    vec3* const color{ (vec3* const) data };
    assert(sizeof(vec3) == size);
    *color = set.color(id);
}

constexpr void get_type(light_set& set, light_id id, void* const data, [[maybe_unused]] u32 size)
{
    graphics::light::type* const type{ (graphics::light::type* const) data };
    assert(sizeof(graphics::light::type) == size);
    *type = set.type(id);
}

constexpr void get_entity_id(light_set& set, light_id id, void* const data, [[maybe_unused]] u32 size)
{
    id::id_type* const entity_id{ (id::id_type* const) data };
    assert(sizeof(id::id_type) == size);
    *entity_id = set.entity_id(id);
}

constexpr void dummy_set(light_set&, light_id, const void* const, u32) {}

using set_function = void (*)(light_set&, light_id, const void* const, u32);
using get_function = void (*)(light_set&, light_id, void* const, u32);
constexpr set_function set_functions[]{
    set_is_enabled, set_intensity, set_color, dummy_set, dummy_set,
};

static_assert(_countof(set_functions) == light_parameter::count);

constexpr get_function get_functions[]{
    get_is_enabled, get_intensity, get_color, get_type, get_entity_id,
};

static_assert(_countof(get_functions) == light_parameter::count);

} // anonymous namespace

graphics::light create(light_init_info info)
{
    assert(id::is_valid(info.entity_id));
    return light_sets[info.light_set_key].add(info);
}

void remove(light_id id, u64 light_set_key)
{
    light_sets[light_set_key].remove(id);
}

void set_parameter(light_id id, u64 light_set_key, light_parameter::parameter param, const void* const data, u32 data_size)
{
    assert(data && data_size);
    assert(param < light_parameter::count && set_functions[param] != dummy_set);
    set_functions[param](light_sets[light_set_key], id, data, data_size);
}

void get_parameter(light_id id, u64 light_set_key, light_parameter::parameter param, void* const data, u32 data_size)
{
    assert(data && data_size);
    assert(param < light_parameter::count);
    get_functions[param](light_sets[light_set_key], id, data, data_size);
}

} // namespace lotus::graphics::d3d12::light