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
// File Name: Window.cpp
// Date File Created: 8/28/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#include "Window.h"


namespace lotus::platform
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Window Class Implementations ////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void Window::SetFullscreen(bool fullscreen) const { }

bool Window::IsFullscreen() const { return false; }

void Window::SetCaption(const char* caption) const { }

vec2i Window::Size() const { return {}; }

void Window::Resize(uint32 width, uint32 height) const { }

const uint32 Window::Width() const { return 0; }

const uint32 Window::Height() const { return 0; }

bool Window::IsClosed() const { return false; }

void* Window::Handle() const { return nullptr; }
} // namespace lotus::platform
