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

#include "../Common.h"
#include <DirectXMath.h>

// Math types

// SIMD variant of vector
using vec = DirectX::XMVECTOR;

// SIMD variant of matrix
using mat = DirectX::XMMATRIX;

using fvec = DirectX::FXMVECTOR;
using cvec = DirectX::CXMVECTOR;

using fmat = DirectX::FXMMATRIX;
using cmat = DirectX::CXMMATRIX;

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

constexpr f32 dx_pi = DirectX::XM_PI;

constexpr f32 pi      = 3.1415926535897932384626433832795f;
constexpr f32 two_pi  = 2.0f * pi;
constexpr f32 epsilon = 1e-5f;

namespace lotus::math
{

template<typename T>
constexpr T clamp(T value, T min, T max)
{
    return value < min ? min : value > max ? max : value;
}


template<u32 Bits>
constexpr u32 pack_unit_float(f32 f)
{
    static_assert(Bits <= sizeof(u32) * 8);
    LASSERT(f >= 0.0f && f <= 1.0f);
    constexpr f32 intervals = (f32) ((1ui32 << Bits) - 1);
    return (u32) (intervals * f + 0.5f);
}

template<u32 Bits>
constexpr f32 unpack_to_unit_float(u32 i)
{
    static_assert(Bits <= sizeof(u32) * 8);
    LASSERT(i < 1ui32 << Bits);
    constexpr f32 intervals = (f32) ((1ui32 << Bits) - 1);
    return (f32) i / intervals;
}

template<u32 Bits>
constexpr u32 pack_float(f32 f, f32 min, f32 max)
{
    LASSERT(min < max);
    LASSERT(f >= min && f <= max);
    const f32 distance = (f - min) / (max - min);
    return pack_unit_float<Bits>(distance);
}

template<u32 Bits>
constexpr f32 unpack_to_float(u32 i, f32 min, f32 max)
{
    LASSERT(min < max);
    return unpack_to_unit_float<Bits>(i) * (max - min) + min;
}

/**
 * \brief Aligns by rounding up. Results in a multiple of Alignment bytes.
 * \tparam Alignment The multiple of bytes to align to
 * \param size The size to be aligned
 * \return The aligned size
 */
template<u64 Alignment>
constexpr u64 align_size_up(u64 size)
{
    static_assert(Alignment, "Alignment must be non-zero");
    constexpr u64 mask = Alignment - 1;
    static_assert(!(Alignment & mask),  "Alignment must be a power of 2");

    return (size + mask) & ~mask;
}

/**
 * \brief Aligns by rounding down. Results in a multiple of Alignment bytes.
 * \tparam Alignment The multiple of bytes to align to
 * \param size The size to be aligned
 * \return The aligned size
 */
template<u64 Alignment>
constexpr u64 align_size_down(u64 size)
{
    static_assert(Alignment, "Alignment must be non-zero");
    constexpr u64 mask = Alignment - 1;
    static_assert(!(Alignment & mask), "Alignment must be a power of 2");

    return size & ~mask;
}


[[nodiscard]] constexpr u64 calculate_crc32_u64(const u8* const data, u64 size)
{
    LASSERT(size >= sizeof(u64));

    u64 crc = 0;
    const u8* at = data;
    const u8* const end = data + align_size_down<sizeof(u64)>(size);

    while(at < end)
    {
        crc = _mm_crc32_u64(crc, *(const u64*)at);
        at+= sizeof(u64);
    }

    return crc;
}


// clang-format off
// TODO: Might just get rid of these... even if I do support Vulkan at some point, I'll probably still use DX math anyway

inline vec load_float(const f32* src) { return DirectX::XMLoadFloat(src); }
inline vec load_float2(const vec2* src) { return DirectX::XMLoadFloat2(src); }
inline vec load_float3(const vec3* src) { return DirectX::XMLoadFloat3(src); }
inline vec load_float4(const vec4* src) { return DirectX::XMLoadFloat4(src); }

inline vec load_float2a(const vec2a* src) { return DirectX::XMLoadFloat2A(src); }
inline vec load_float3a(const vec3a* src) { return DirectX::XMLoadFloat3A(src); }
inline vec load_float4a(const vec4a* src) { return DirectX::XMLoadFloat4A(src); }

inline void store_float(f32* dest, fvec v) { DirectX::XMStoreFloat(dest, v); }
inline void store_float2(vec2* dest, fvec v) { DirectX::XMStoreFloat2(dest, v); }
inline void store_float3(vec3* dest, fvec v) { DirectX::XMStoreFloat3(dest, v); }
inline void store_float4(vec4* dest, fvec v) { DirectX::XMStoreFloat4(dest, v); }

inline void store_float2a(vec2a* dest, fvec v) { DirectX::XMStoreFloat2A(dest, v); }
inline void store_float3a(vec3a* dest, fvec v) { DirectX::XMStoreFloat3A(dest, v); }
inline void store_float4a(vec4a* dest, fvec v) { DirectX::XMStoreFloat4A(dest, v); }

inline void store_float4x4(mat4* dest, fmat m) { DirectX::XMStoreFloat4x4(dest, m); }

inline vec normalize_vec3(fvec v) { return DirectX::XMVector3Normalize(v); }
inline vec dot_vec3(fvec v1, fvec v2) { return DirectX::XMVector3Dot(v1, v2); }
inline vec cross_vec3(fvec v1, fvec v2) { return DirectX::XMVector3Cross(v1, v2); }
inline vec reciprocal_length_vec3(fvec v1) { return DirectX::XMVector3ReciprocalLength(v1); }

inline f32  scalar_cos(f32 value) { return DirectX::XMScalarCos(value); }
inline f32 scalar_sin(f32 value) { return DirectX::XMScalarSin(value); }
inline bool scalar_near_equal(f32 s1, f32 s2) { return DirectX::XMScalarNearEqual(s1, s2, epsilon); }

inline vec rotate_vec3(fvec v, fvec quat) { return DirectX::XMVector3Rotate(v, quat); }

inline vec quat_rotation_roll_pitch_yaw_from_vec(vec v) {return DirectX::XMQuaternionRotationRollPitchYawFromVector(v); }

inline mat look_to_rh(fvec eye_pos, fvec focus_pos, fvec up_dir) { return DirectX::XMMatrixLookToRH(eye_pos, focus_pos, up_dir); }

inline mat perspective_fov_rh(f32 angle_y, f32 aspect_ratio, f32 near_z, f32 far_z) { return DirectX::XMMatrixPerspectiveFovRH(angle_y, aspect_ratio, near_z, far_z); }

inline mat orthographic_rh(f32 width, f32 height, f32 near_z, f32 far_z) { return DirectX::XMMatrixOrthographicRH(width, height, near_z, far_z); }

inline mat inverse_matrix(vec* determinant, fmat m) { return DirectX::XMMatrixInverse(determinant, m); }

inline mat mul_matrix(fmat a, cmat b) { return DirectX::XMMatrixMultiply(a, b); }

inline vec set_vector(f32 x, f32 y, f32 z, f32 w) { return DirectX::XMVectorSet(x, y, z, w); }

// clang-format on

} // namespace lotus::math
