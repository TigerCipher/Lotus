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
// File Name: D3D12Common.h
// Date File Created: 10/30/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "Lotus/Core/Common.h"
#include "Lotus/Graphics/Renderer.h"
#include "Lotus/Platform/Window.h"

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")


#ifdef L_DEBUG
    #ifndef DX_CALL
        #define DX_CALL(x)                                                                                             \
            if (FAILED(x))                                                                                             \
            {                                                                                                          \
                char line_number[32];                                                                                  \
                sprintf_s(line_number, "%u", __LINE__);                                                                \
                OutputDebugStringA("ERROR IN: ");                                                                      \
                OutputDebugStringA(__FILE__);                                                                          \
                OutputDebugStringA("\nLine: ");                                                                        \
                OutputDebugStringA(line_number);                                                                       \
                OutputDebugStringA("\n");                                                                              \
                OutputDebugStringA(#x);                                                                                \
                OutputDebugStringA("\n");                                                                              \
                __debugbreak();                                                                                        \
            }
    #endif

    #define NAME_D3D_OBJ(obj, name)                                                                                    \
        obj->SetName(name);                                                                                            \
        OutputDebugString(L"::D3D12 Object Created: ");                                                                \
        OutputDebugString(name);                                                                                       \
        OutputDebugString(L"\n")

    #define NAME_D3D_OBJ_INDEXED(obj, i, name)                                                                         \
        {                                                                                                              \
            wchar_t fullname[128];                                                                                     \
            if (swprintf(fullname, L"%s[%u]", name, i) > 0)                                                            \
            {                                                                                                          \
                obj->SetName(name);                                                                                    \
                OutputDebugString(L"::D3D12 Object Created: ");                                                        \
                OutputDebugString(fullname);                                                                           \
                OutputDebugString(L"\n");                                                                              \
            }                                                                                                          \
        }

#else
    #ifndef DX_CALL
        #define DX_CALL(x) x
    #endif
    #define NAME_D3D_OBJ(obj, name)
    #define NAME_D3D_OBJ_INDEXED(obj, i, name)
#endif

template<typename T>
using comptr = Microsoft::WRL::ComPtr<T>;

template<typename T>
using cptr = Microsoft::WRL::ComPtr<T>;

template<typename T, typename... Args>
constexpr cptr<T> create_com(Args&&... args)
{
    return Microsoft::WRL::Make<T>(std::forward<Args>(args)...);
}

constexpr u32 frame_buffer_count = 3;