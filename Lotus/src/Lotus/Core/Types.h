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
// File Name: Types.h
// Date File Created: 8/28/2022
// Author: Matt
// 
// ------------------------------------------------------------------------------
#pragma once
#include <cstdint>
#include <memory>
#include <string>

// Types and shortcuts

namespace lotus
{
// byte is contained in the lotus namespace, because I've had issues with it conflicting with std::byte in other
// projects
using byte = uint8_t;
} // namespace lotus
using ulong = unsigned long;

// ensure x bit, but I hate using _t, it looks ugly

using uint8  = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8  = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;


// Even shorter because why not? (above ones are preferred for explicitness)
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;


constexpr uint8  invalid_id_u8  = 0xffui8;
constexpr uint16 invalid_id_u16 = 0xffffui16;
constexpr uint32 invalid_id_u32 = 0xffff'ffffui32;
constexpr uint64 invalid_id_u64 = 0xffff'ffff'ffff'ffffui64;


using f32 = float;
using f64 = double;


using string_hash = std::hash<std::string>;


template<typename T>
using UniquePtr = std::unique_ptr<T>;

// Scope as well as above in case I decide to make my own version of unique ptr
template<typename T>
using Scope = std::unique_ptr<T>;

template<typename T>
using SharedPtr = std::shared_ptr<T>;

// Ref as well as above in case I decide to make my own version of shared ptr
template<typename T>
using Ref = std::shared_ptr<T>;


template<typename T, typename... Args>
constexpr Scope<T> CreateScope(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}


