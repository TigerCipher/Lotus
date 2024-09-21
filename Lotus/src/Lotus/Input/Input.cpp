// ------------------------------------------------------------------------------
//
// Lotus
// Copyright 2024 Matthew Rogers
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// File Name: Input.cpp
// Date File Created: 09/21/2024
// Author: Matt
//
// ------------------------------------------------------------------------------


#include "Input.h"

namespace lotus::input
{
namespace
{
struct input_binding
{
    utl::vector<input_source> sources;
    input_value               value{};
    bool                      is_dirty{ true };
};

std::unordered_map<u64, input_binding>  input_bindings;
std::unordered_map<u64, u64>            source_binding_map;
std::unordered_map<u64, input_value>    input_values;
utl::vector<detail::input_system_base*> input_callbacks;

constexpr u64 get_key(input_source::type type, u32 code)
{
    return ((u64) type << 32 | (u64) code);
}

} // anonymous namespace

void bind(input_source source)
{
    assert(source.source_type < input_source::count);
    const u64 key = get_key(source.source_type, source.code);
    unbind(source.source_type, (input_code::code) source.code);
    input_bindings[source.binding].sources.emplace_back(source);
    source_binding_map[key] = source.binding;
}

void unbind(input_source::type type, input_code::code code)
{
    assert(type < input_source::count);
    const u64 key = get_key(type, code);
    if (!source_binding_map.contains(key))
    {
        return;
    }
    const u64 binding_key = source_binding_map[key];
    assert(input_bindings.count(binding_key));
    input_binding&             binding = input_bindings[binding_key];
    utl::vector<input_source>& sources = binding.sources;
    u32                        index   = invalid_id_u32;
    for (u32 i = 0; i < sources.size(); ++i)
    {
        if (sources[i].source_type == type && sources[i].code == code)
        {
            assert(sources[i].binding == source_binding_map[key]);
            index = i;
            break;
        }
    }
    if (index != invalid_id_u32)
    {
        utl::erase_unordered(sources, index);
        source_binding_map.erase(key);
    }
    if (!sources.size())
    {
        assert(!source_binding_map.contains(key));
        input_bindings.erase(binding_key);
    }
}
void unbind(u64 binding)
{
    if (!input_bindings.contains(binding))
    {
        return;
    }
    utl::vector<input_source>& sources{ input_bindings[binding].sources };
    for (const auto& source : sources)
    {
        assert(source.binding == binding);
        const u64 key = get_key(source.source_type, source.code);
        assert(source_binding_map.contains(key) && source_binding_map[key] == binding);
        source_binding_map.erase(key);
    }
    input_bindings.erase(binding);
}

void set(input_source::type type, input_code::code code, vec3 value)
{
    assert(type < input_source::count);

    const u64    key   = get_key(type, code);
    input_value& input = input_values[key];
    input.previous     = input.current;
    input.current      = value;

    if (source_binding_map.contains(key))
    {
        const u64 binding_key = source_binding_map[key];
        assert(input_bindings.contains(binding_key));
        input_binding& binding = input_bindings[binding_key];
        binding.is_dirty       = true;

        input_value binding_value;
        get(binding_key, binding_value);

        // TODO: these callbacks can cause data race conditions in scripts when not run on same thread as game scripts
        for (const auto& callback : input_callbacks)
        {
            callback->on_event(binding_key, binding_value);
        }
    }

    // TODO: these callbacks can cause data race conditions in scripts when not run on same thread as game scripts
    for (const auto& callback : input_callbacks)
    {
        callback->on_event(type, code, input);
    }
}

void get(input_source::type type, input_code::code code, input_value& value)
{
    assert(type < input_source::count);

    const u64 key = get_key(type, code);
    value         = input_values[key];
}

void get(u64 binding, input_value& value)
{
    if (!input_bindings.contains(binding))
    {
        return;
    }
    input_binding& input_binding = input_bindings[binding];
    if (!input_binding.is_dirty)
    {
        value = input_binding.value;
        return;
    }
    utl::vector<input_source>& sources = input_bindings[binding].sources;
    input_value                sub_value{};
    input_value                result{};
    for (const auto& source : sources)
    {
        assert(source.binding == binding);
        get(source.source_type, (input_code::code) source.code, sub_value);
        assert(source.axis <= axis::z);
        if (source.source_type == input_source::mouse)
        {
            const f32 current  = (&sub_value.current.x)[source.source_axis];
            const f32 previous = (&sub_value.previous.x)[source.source_axis];
            (&result.current.x)[source.axis] += (current - previous) * source.multiplier;
        } else
        {
            (&result.previous.x)[source.axis] += sub_value.previous.x * source.multiplier;
            (&result.current.x)[source.axis] += sub_value.current.x * source.multiplier;
        }
    }
    input_binding.value    = result;
    input_binding.is_dirty = false;
    value                  = result;
}

detail::input_system_base::input_system_base()
{
    input_callbacks.emplace_back(this);
}

detail::input_system_base::~input_system_base()
{
    for (u32 i = 0; i < input_callbacks.size(); ++i)
    {
        if (input_callbacks[i] == this)
        {
            utl::erase_unordered(input_callbacks, i);
            break;
        }
    }
}


} // namespace lotus::input