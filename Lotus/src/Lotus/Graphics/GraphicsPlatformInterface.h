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
// File Name: GraphicsPlatformInterface.h
// Date File Created: 10/30/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "Lotus/Core/Common.h"
#include "Renderer.h"
#include "Lotus/Platform/Window.h"

namespace lotus::graphics
{
struct platform_interface
{
    bool (*initialize)(void);
    void (*shutdown)(void);

    struct
    {
        surface (*create)(platform::window);
        void (*remove)(surface_id);
        void (*resize)(surface_id, u32, u32);
        u32 (*width)(surface_id);
        u32 (*height)(surface_id);
        void (*render)(surface_id);
    } surface;
};
} // namespace lotus::graphics