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
// File Name: Window.h
// Date File Created: 8/28/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once
#include "../Core/Common.h"

namespace lotus::platform
{
L_TYPED_ID(window_id)

class window
{
public:
    constexpr window() = default;
    constexpr explicit window(const window_id id) : m_id(id) {}

    constexpr window_id get_id() const { return m_id; }

    constexpr bool is_valid() const { return id::is_valid(m_id); }

    void set_fullscreen(bool fullscreen) const;
    bool is_fullscreen() const;
    void set_caption(const wchar_t* caption) const;

    vec4u rect() const;
    vec2u size() const;
    void  resize(uint32 width, uint32 height) const;

    uint32 width() const;
    uint32 height() const;

    bool is_closed() const;

    void* handle() const;

private:
    window_id m_id{id::invalid_id};
};


} // namespace lotus::platform
