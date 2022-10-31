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
// File Name: Platform.h
// Date File Created: 8/28/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once
#include "Window.h"

namespace lotus::platform
{
// #Consider: After D3D12 should I consider supporting mac/linux with vulkan and metal?

using window_proc   = LRESULT (*)(HWND, UINT, WPARAM, LPARAM);
using window_handle = HWND;

struct window_create_info
{
    window_proc    callback = nullptr;
    window_handle  parent   = nullptr;
    const wchar_t* caption  = nullptr;
    int32          left     = 0;
    int32          top      = 0;
    int32          width    = 1920;
    int32          height   = 1080;
};

window create_window(const window_create_info* const info = nullptr);
void   remove_window(window_id id);
} // namespace lotus::platform
