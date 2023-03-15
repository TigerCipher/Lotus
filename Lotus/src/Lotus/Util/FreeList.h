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
// File Name: FreeList.h
// Date File Created: 01/17/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#pragma once

#include "../Common.h"

template<typename T>
concept min_u32 = sizeof(T) >= sizeof(u32);

namespace lotus::utl
{
#if USE_STL_VECTOR
    #pragma message("Using utl::free_list with std::vector will result in duplicate calls to the class constructor")
#endif


template<min_u32 T>
class free_list
{
    //static_assert(sizeof(T) >= sizeof(u32));

public:
    free_list() = default;
    explicit free_list(u32 count) { m_array.reserve(count); }

    ~free_list()
    {
        assert(!m_size);
#if USE_STL_VECTOR
        memset(m_array.data(), 0, m_array.size() * sizeof(T));
#endif
    }

    template<class... Params>
    constexpr u32 add(Params&&... p)
    {
        u32 id = invalid_id_u32;
        if (m_next_free_index == invalid_id_u32)
        {
            id = (u32) m_array.size();
            m_array.emplace_back(std::forward<Params>(p)...);
        } else
        {
            id = m_next_free_index;
            assert(id < m_array.size() && already_removed(id));
            m_next_free_index = *(const u32* const) std::addressof(m_array[id]);
            new (std::addressof(m_array[id])) T(std::forward<Params>(p)...);
        }
        ++m_size;
        return id;
    }

    constexpr void remove(u32 id)
    {
        assert(id < m_array.size() && !already_removed(id));
        T& item = m_array[id];
        item.~T();
        L_DBG(memset(std::addressof(m_array[id]), 0xcc, sizeof(T)));
        *(u32* const) std::addressof(m_array[id]) = m_next_free_index;

        m_next_free_index = id;
        --m_size;
    }

    [[nodiscard]] constexpr u32 size() const { return m_size; }
    [[nodiscard]] constexpr u32 capacity() const { return m_array.size(); }

    [[nodiscard]] constexpr bool empty() const { return m_size == 0; }

    constexpr T& operator[](u32 id)
    {
        assert(id < m_array.size() && !already_removed(id));
        return m_array[id];
    }

    constexpr const T& operator[](u32 id) const
    {
        assert(id < m_array.size() && !already_removed(id));
        return m_array[id];
    }


private:
    constexpr bool already_removed(u32 id)
    {
        if constexpr (sizeof(T) > sizeof(u32))
        {
            u32        i = sizeof(u32);
            const auto p = (const u8* const) std::addressof(m_array[id]);
            while (p[i] == 0xcc && i < sizeof(T))
                ++i;
            return i == sizeof(T);
        } else
            return true;
    }

#if USE_STL_VECTOR
    utl::vector<T> m_array;
#else
    utl::vector<T, false> m_array;
#endif

    u32 m_next_free_index{ invalid_id_u32 };
    u32 m_size{ 0 };
};

} // namespace lotus::utl