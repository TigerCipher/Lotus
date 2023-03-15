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
#include "Types.h"


namespace lotus::id
{

using id_type = u32;
constexpr size_t size = sizeof(id_type);

namespace detail
{
constexpr u32     generation_bits = 10;
constexpr u32     index_bits      = sizeof(id_type) * 8 - generation_bits;
constexpr id_type index_mask      = (id_type{ 1 } << index_bits) - 1;
constexpr id_type generation_mask = (id_type{ 1 } << generation_bits) - 1;
} // namespace detail

#pragma warning(disable : 4245)
constexpr id_type invalid_id           = -1;
constexpr u32     min_deleted_elements = 1024;

using gen_type =
    std::conditional_t<detail::generation_bits <= 16, std::conditional_t<detail::generation_bits <= 8, u8, u16>, u32>;

static_assert(sizeof(gen_type) * 8 >= detail::generation_bits);
static_assert(sizeof(id_type) - sizeof(gen_type) > 0);

constexpr bool is_valid(const id_type id)
{
    return id != invalid_id;
}

constexpr id_type index(const id_type id)
{
    assert((id & detail::index_mask) != detail::index_mask);
    return id & detail::index_mask;
}

constexpr id_type generation(const id_type id)
{
    return (id >> detail::index_bits) & detail::generation_mask;
}

constexpr id_type new_generation(const id_type id)
{
    const id_type gen = generation(id) + 1;
    assert(gen < ((u64) 1 << detail::generation_bits) - 1);
    return index(id) | (gen << detail::index_bits);
}


#ifdef L_DEBUG
namespace detail
{
struct id_base
{
    constexpr explicit id_base(const id_type id) : m_id(id) {}
    constexpr operator id_type() const { return m_id; }

private:
    id_type m_id;
};
} // namespace detail

    #define L_TYPED_ID(name)                                                                                                \
        struct name final : id::detail::id_base                                                                             \
        {                                                                                                                   \
            constexpr explicit name(const id::id_type id) : id_base(id)                                                     \
            {}                                                                                                              \
            constexpr name() : id_base(0)                                                                                   \
            {}                                                                                                              \
        };

#else
    #define L_TYPED_ID(name) using name = id::id_type;
#endif

} // namespace lotus::id
