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

#include "../../Common.h"
#include "../../Graphics/Renderer.h"
#include "../../Platform/Window.h"

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")


#ifdef L_DEBUG
    #ifndef DX_CALL
        #define DX_CALL(x)                                                                                                       \
            if (FAILED(x))                                                                                                       \
            {                                                                                                                    \
                char line_number[32];                                                                                            \
                sprintf_s(line_number, "%u", __LINE__);                                                                          \
                OutputDebugStringA("ERROR IN: ");                                                                                \
                OutputDebugStringA(__FILE__);                                                                                    \
                OutputDebugStringA("\nLine: ");                                                                                  \
                OutputDebugStringA(line_number);                                                                                 \
                OutputDebugStringA("\n");                                                                                        \
                OutputDebugStringA(#x);                                                                                          \
                OutputDebugStringA("\n");                                                                                        \
                __debugbreak();                                                                                                  \
            }
    #endif

    #define NAME_D3D_OBJ(obj, name)                                                                                              \
        obj->SetName(name);                                                                                                      \
        OutputDebugString(L"================= D3D12 Object Created: ");                                                          \
        OutputDebugString(name);                                                                                                 \
        OutputDebugString(L"\n")

    #define NAME_D3D_OBJ_INDEXED(obj, i, name)                                                                                   \
        {                                                                                                                        \
            wchar_t fullname[128];                                                                                               \
            if (swprintf_s(fullname, L"%s[%llu]", name, (u64) i) > 0)                                                            \
            {                                                                                                                    \
                (obj)->SetName(name);                                                                                            \
                OutputDebugString(L"================= D3D12 Object Created: ");                                                  \
                OutputDebugString(fullname);                                                                                     \
                OutputDebugString(L"\n");                                                                                        \
            }                                                                                                                    \
        }

#else
    #ifndef DX_CALL
        #define DX_CALL(x) x
    #endif
    #define NAME_D3D_OBJ(obj, name)
    #define NAME_D3D_OBJ_INDEXED(obj, i, name)
#endif


// shorthands for some common things I use that I'm too lazy to keep rewriting, or I think look ugly

#define L_PTR(x) IID_PPV_ARGS((x))

#define L_HRES(hr) HRESULT hr = S_OK

// microsoft smart pointer

template<typename T>
using comptr = Microsoft::WRL::ComPtr<T>;


template<typename T, typename... Args>
constexpr comptr<T> create_com(Args&&... args)
{
    return Microsoft::WRL::Make<T>(std::forward<Args>(args)...);
}

namespace lotus::graphics::d3d12
{
constexpr u32 frame_buffer_count = 3;

using id3d12_device                = ID3D12Device10;
using id3d12_graphics_command_list = ID3D12GraphicsCommandList7;

} // namespace lotus::graphics::d3d12


#include "D3D12Helpers.h"
#include "D3D12Resources.h"