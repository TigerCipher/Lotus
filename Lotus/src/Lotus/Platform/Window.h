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
#include "Lotus/Core/Common.h"

namespace lotus::platform
{
L_TYPED_ID(window_id)

class Window
{
public:
    constexpr Window() = default;
    constexpr explicit Window(const window_id id) : mId(id) { }

    constexpr window_id GetId() const { return mId; }

    constexpr bool IsValid() const { return id::is_valid(mId); }

    void SetFullscreen(bool fullscreen) const;
    bool IsFullscreen() const;
    void SetCaption(const wchar_t* caption) const;

    vec4u Rect() const;
    vec2u Size() const;
    void  Resize(uint32 width, uint32 height) const;

    const uint32 Width() const;
    const uint32 Height() const;

    bool IsClosed() const;

    void* Handle() const;

private:
    window_id mId { id::InvalidId };
};


} // namespace lotus::platform
