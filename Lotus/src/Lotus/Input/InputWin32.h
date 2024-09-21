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
// File Name: InputWin32.h
// Date File Created: 09/21/2024
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#ifdef _WIN64

    #include "Common.h"

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>

namespace lotus::input
{
HRESULT process_input_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
} // namespace lotus::input

#endif