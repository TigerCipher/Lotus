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
// File Name: MathUtil.h
// Date File Created: 8/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "../Core/Common.h"
#include <DirectXMath.h>

// Math types

using vec = DirectX::XMVECTOR;

// hlsl style naming
using float2  = DirectX::XMFLOAT2;
using float2a = DirectX::XMFLOAT2A;
using float3  = DirectX::XMFLOAT3;
using float3a = DirectX::XMFLOAT3A;
using float4  = DirectX::XMFLOAT4;
using float4a = DirectX::XMFLOAT4A;
using uint2v  = DirectX::XMUINT2;
using uint3v  = DirectX::XMUINT3;
using uint4v  = DirectX::XMUINT4;
using int2v   = DirectX::XMINT2;
using int3v   = DirectX::XMINT3;
using int4v   = DirectX::XMINT4;
using mat3x3  = DirectX::XMFLOAT3X3;
using mat4x4  = DirectX::XMFLOAT4X4;
using mat4x4a = DirectX::XMFLOAT4X4A;

// glsl style naming (usually what I prefer)
using vec2  = DirectX::XMFLOAT2;
using vec2a = DirectX::XMFLOAT2A;
using vec3  = DirectX::XMFLOAT3;
using vec3a = DirectX::XMFLOAT3A;
using vec4  = DirectX::XMFLOAT4;
using vec4a = DirectX::XMFLOAT4A;
using vec2u = DirectX::XMUINT2;
using vec3u = DirectX::XMUINT3;
using vec4u = DirectX::XMUINT4;
using vec2i = DirectX::XMINT2;
using vec3i = DirectX::XMINT3;
using vec4i = DirectX::XMINT4;
using mat3  = DirectX::XMFLOAT3X3;
using mat4  = DirectX::XMFLOAT4X4;
using mat4a = DirectX::XMFLOAT4X4A;

constexpr float pi      = 3.1415926535897932384626433832795f;
constexpr float epsilon = 1e-5f;

namespace lotus::math
{
// #TODO: Math helpers
}
