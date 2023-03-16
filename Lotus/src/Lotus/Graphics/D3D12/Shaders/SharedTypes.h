// ------------------------------------------------------------------------------
//
// Lotus
//    Copyright 2023 Matthew Rogers
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
// File Name: SharedTypes
// Date File Created: 3/15/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#pragma once


#include "Graphics/D3D12/D3D12Common.h"

namespace lotus::graphics::d3d12::hlsl
{
using uint     = u32;
using uint2    = vec2u;
using uint3    = vec3u;
using float4   = vec4;
using float3   = vec3;
using float2   = vec2;
using float4x4 = mat4a;
#include "CommonTypes.hlsli"
} // namespace lotus::graphics::d3d12::hlsl
