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
// File Name: Common.h
// Date File Created: 08/19/2022
// Author: Matt
//
// ------------------------------------------------------------------------------


// ReSharper disable CppInconsistentNaming
#pragma once

// Disable warnings
#pragma warning(disable : 4530)


// Macros

// Make sure L_DEBUG is defined for debug builds
// TODO: Really should just use _DEBUG like conventional
#if !defined(L_DEBUG) && defined(_DEBUG)
    #define L_DEBUG
#endif

#ifdef L_EDITOR
    #define L_EXPORT extern "C" __declspec(dllexport)
#else
    #define L_EXPORT
#endif


// Macro helpers
#define NO_INLINE           __declspec(noinline)
#define LEXPAND_MACRO(x)    x
#define LSTRINGIFY_MACRO(x) #x
#define BIT(x)              (1 << (x))
#define LBIND_EVENT_FUNC(func)                                                                                                   \
    [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }
#define LDELETE(x)                                                                                                               \
    delete (x);                                                                                                                  \
    (x) = nullptr;

#ifdef L_DEBUG
    #define L_DBG(x) x
#else
    #define L_DBG(x)
#endif

#ifndef DISABLE_COPY
    #define DISABLE_COPY(T)                                                                                                      \
        explicit T(const T&)   = delete;                                                                                         \
        T& operator=(const T&) = delete
#endif

#ifndef DISABLE_MOVE
    #define DISABLE_MOVE(T)                                                                                                      \
        explicit T(T&&)   = delete;                                                                                              \
        T& operator=(T&&) = delete
#endif

#ifndef DISABLE_COPY_AND_MOVE
    #define DISABLE_COPY_AND_MOVE(T)                                                                                             \
        DISABLE_COPY(T);                                                                                                         \
        DISABLE_MOVE(T)

#endif

// System includes

#include <cstdint>
#include <memory>
#include <utility>
#include <cassert>
#include <unordered_map>
#include <mutex>

// #include <array>
// #include <map>
// #include <functional>
// #include <algorithm>
// #include <string>
// #include <sstream>


#ifdef _WIN64
    #define WIN32_LEAN_AND_MEAN
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>
#else
    #error Currently only Windows x64 is supported. No current plans to change this
#endif


// Lotus includes

#include "Core/Id.h"
#include "Core/Types.h"
#include "Util/Util.h"
#include "Util/MathUtil.h"
#include "Util/Logger.h"

#if defined(_DEBUG) && !defined(L_EDITOR)
    #define LOG_TRACE(msg, ...) lotus::logger::detail::output(lotus::logger::log_level::trace, std::format(msg, ##__VA_ARGS__))
    #define LOG_DEBUG(msg, ...) lotus::logger::detail::output(lotus::logger::log_level::debug, std::format(msg, ##__VA_ARGS__))
    #define LOG_INFO(msg, ...)  lotus::logger::detail::output(lotus::logger::log_level::info, std::format(msg, ##__VA_ARGS__))
    #define LOG_WARN(msg, ...)  lotus::logger::detail::output(lotus::logger::log_level::warn, std::format(msg, ##__VA_ARGS__))
    #define LOG_ERROR(msg, ...) lotus::logger::detail::output(lotus::logger::log_level::error, std::format(msg, ##__VA_ARGS__))
    #define LOG_FATAL(msg, ...) lotus::logger::detail::output(lotus::logger::log_level::fatal, std::format(msg, ##__VA_ARGS__))
#else
    #define LOG_TRACE(msg, ...)
    #define LOG_DEBUG(msg, ...)
    #define LOG_INFO(msg, ...)
    #define LOG_WARN(msg, ...)
    #define LOG_ERROR(msg, ...)
    #define LOG_FATAL(msg, ...)
#endif