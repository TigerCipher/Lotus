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
// File Name: Id.h
// Date File Created: 8/19/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "Common.h"

#include <cassert>

namespace lotus::id
{

using id_type = uint32;

namespace detail
{
    constexpr uint32  GenerationBits = 10;
    constexpr uint32  IndexBits      = sizeof(id_type) * 8 - GenerationBits;
    constexpr id_type IndexMask      = (id_type { 1 } << IndexBits) - 1;
    constexpr id_type GenerationMask = (id_type { 1 } << GenerationBits) - 1;
} // namespace detail

#pragma warning(disable : 4245)
constexpr id_type InvalidId          = -1;
constexpr uint32  MinDeletedElements = 1024;

using gen_type =
    std::conditional_t<detail::GenerationBits <= 16, std::conditional_t<detail::GenerationBits <= 8, u8, u16>, u32>;

static_assert(sizeof(gen_type) * 8 >= detail::GenerationBits);
static_assert(sizeof(id_type) - sizeof(gen_type) > 0);

constexpr bool IsValid(const id_type id) { return id != InvalidId; }

constexpr id_type Index(const id_type id)
{
    id_type i = id & detail::IndexMask;
    LASSERT(i != detail::IndexMask);
    return id & detail::IndexMask;
}

constexpr id_type Generation(const id_type id) { return (id >> detail::IndexBits) & detail::GenerationMask; }

constexpr id_type NewGeneration(const id_type id)
{
    const id_type gen = Generation(id) + 1;
    LASSERT(gen < ((u64) 1 << detail::GenerationBits) - 1);
    return Index(id) | (gen << detail::IndexBits);
}


#ifdef L_DEBUG
namespace detail
{
    struct IdBase
    {
        constexpr explicit IdBase(id_type id) : mId(id) { }
        constexpr operator id_type() const { return mId; }

    private:
        id_type mId;
    };
} // namespace detail

    #define L_TYPED_ID(name)                                                                                           \
        struct name final : id::detail::IdBase                                                                         \
        {                                                                                                              \
            constexpr explicit name(id::id_type id) : IdBase(id) { }                                                   \
            constexpr name() : IdBase(0) { }                                                                           \
        };

#else
    #define L_TYPED_ID(name) using name = id_type;
#endif

} // namespace lotus::id
