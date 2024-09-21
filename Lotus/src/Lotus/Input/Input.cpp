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
std::unordered_map<u64, input_value>    input_values;
utl::vector<detail::input_system_base*> input_callbacks;

constexpr u64 get_key(input_source::type type, u32 code)
{
    return ((u64) type << 32 | (u64) code);
}

} // anonymous namespace

void set(input_source::type type, input_code::code code, vec3 value)
{
    assert(type < input_source::count);

    const u64    key   = get_key(type, code);
    input_value& input = input_values[key];
    input.previous     = input.current;
    input.current      = value;

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