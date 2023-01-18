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
// File Name: Vector.h
// Date File Created: 01/17/2023
// Author: Matt
//
// ------------------------------------------------------------------------------

#pragma once

#include "Lotus/Core/Common.h"

namespace lotus::utl
{

template<typename T, bool destruct = true>
class vector
{
public:
    vector() = default;

    constexpr explicit vector(u64 count) { resize(count); }

    constexpr explicit vector(u64 count, const T& value) { resize(count, value); }

    template<typename it, typename = std::enable_if_t<std::_Is_iterator_v<it>>>
    constexpr explicit vector(it first, it last)
    {
        for (; first != last; ++first)
        {
            emplace_back(*first);
        }
    }

    // copy ctor
    constexpr vector(const vector& o) { *this = o; }

    // move ctor
    constexpr vector(const vector&& o) : m_capacity(o.m_capacity), m_size(o.m_size), m_data(o.m_data) { o.reset(); }

    ~vector() { destroy(); }

    constexpr vector& operator=(const vector& o)
    {
        LASSERT(this != std::addressof(o));
        if (this != std::addressof(o))
        {
            clear();
            reserve(o.m_size);
            for (auto& item : o)
            {
                emplace_back(item);
            }
            LASSERT(m_size == o.m_size);
        }

        return *this;
    }

    constexpr vector& operator=(vector&& o)
    {
        LASSERT(this != std::addressof(o));
        if (this != std::addressof(o))
        {
            destroy();
            move(o);
        }

        return *this;
    }

    constexpr void push_back(const T& value) { emplace_back(value); }

    constexpr void push_back(T&& value) { emplace_back(std::move(value)); }

    // Inserts items to the end of the vector
    template<typename... Params>
    constexpr decltype(auto) emplace_back(Params&&... p)
    {
        if (m_size == m_capacity)
        {
            reserve((m_capacity + 1) * 3 >> 1); // increases capacity by 50%
        }
        LASSERT(m_size < m_capacity);

        new (std::addressof(m_data[m_size])) T(std::forward<Params>(p)...);
        ++m_size;
        return m_data[m_size - 1];
    }

    constexpr void resize(u64 new_size)
    {
        static_assert(std::is_default_constructible_v<T>, "Type must have a default constructor");

        if (new_size > m_size)
        {
            reserve(new_size);
            while (m_size < new_size)
            {
                emplace_back();
            }
        } else if (new_size < m_size)
        {
            if constexpr (destruct)
            {
                destruct_range(new_size, m_size);
            }
        }

        LASSERT(new_size == m_size);
    }

    constexpr void resize(u64 new_size, const T& value)
    {
        static_assert(std::is_copy_constructible_v<T>, "Type must be copyable");

        if (new_size > m_size)
        {
            reserve(new_size);
            while (m_size < new_size)
            {
                emplace_back(value);
            }
        } else if (new_size < m_size)
        {
            if constexpr (destruct)
            {
                destruct_range(new_size, m_size);
            }
        }

        LASSERT(new_size == m_size);
    }

    constexpr void reserve(u64 new_capacity)
    {
        if (new_capacity <= m_capacity)
            return;

        void* new_buffer = realloc(m_data, new_capacity * sizeof(T));
        LASSERT(new_buffer);
        if (new_buffer)
        {
            m_data     = static_cast<T*>(new_buffer);
            m_capacity = new_capacity;
        }
    }

    constexpr T* const erase(u64 index)
    {
        LASSERT(m_data && index < m_size);
        return erase(std::addressof(m_data[index]));
    }

    constexpr T* const erase(T* const item)
    {
        LASSERT(m_data && item >= std::addressof(m_data[0]) && item < std::addressof(m_data[m_size]));

        if constexpr (destruct)
            item->~T();
        --m_size;
        if (item < std::addressof(m_data[m_size]))
        {
            memcpy(item, item + 1, (std::addressof(m_data[m_size]) - item) * sizeof(T));
        }

        return item;
    }

    constexpr T* const erase_unordered(u64 index)
    {
        LASSERT(m_data && index < m_size);
        return erase_unordered(std::addressof(m_data[index]));
    }

    constexpr T* const erase_unordered(T* const item)
    {
        LASSERT(m_data && item >= std::addressof(m_data[0]) && item < std::addressof(m_data[m_size]));

        if constexpr (destruct)
            item->~T();
        --m_size;
        if (item < std::addressof(m_data[m_size]))
        {
            memcpy(item, std::addressof(m_data[m_size]), sizeof(T));
        }

        return item;
    }

    constexpr void clear()
    {
        if constexpr (destruct)
        {
            destruct_range(0, m_size);
        }
        m_size = 0;
    }

    constexpr void swap(vector& o)
    {
        if (this == std::addressof(o))
            return;
        auto temp(o);
        o     = *this;
        *this = temp;
    }

    constexpr T* data() { return m_data; }

    constexpr T* const data() const { return m_data; }

    constexpr bool empty() const { return m_size == 0; }

    constexpr u64 size() const { return m_size; }

    constexpr u64 capacity() const { return m_capacity; }

    constexpr T& operator[](u64 index)
    {
        LASSERT(m_data && index < m_size);
        return m_data[index];
    }

    constexpr const T& operator[](u64 index) const
    {
        LASSERT(m_data && index < m_size);
        return m_data[index];
    }

    constexpr T& front()
    {
        LASSERT(m_data && m_size);
        return m_data[0];
    }

    constexpr const T& front() const
    {
        LASSERT(m_data && m_size);
        return m_data[0];
    }

    constexpr T& back()
    {
        LASSERT(m_data && m_size);
        return m_data[m_size - 1];
    }

    constexpr const T& back() const
    {
        LASSERT(m_data && m_size);
        return m_data[m_size - 1];
    }

    constexpr T* begin()
    {
        LASSERT(m_data);
        return std::addressof(m_data[0]);
    }

    constexpr const T* begin() const
    {
        LASSERT(m_data);
        return std::addressof(m_data[0]);
    }

    constexpr T* end()
    {
        LASSERT(m_data);
        return std::addressof(m_data[m_size]);
    }

    constexpr const T* end() const
    {
        LASSERT(m_data);
        return std::addressof(m_data[m_size]);
    }

private:
    constexpr void move(vector& o)
    {
        m_capacity = o.m_capacity;
        m_size     = o.m_size;
        m_data     = o.m_data;
        o.reset();
    }

    constexpr void reset()
    {
        m_capacity = 0;
        m_size     = 0;
        m_data     = nullptr;
    }

    constexpr void destruct_range(u64 first, u64 last)
    {
        LASSERT(destruct);
        LASSERT(first <= m_size && last <= m_size && first <= last);
        if (!m_data)
            return;

        for (; first != last; ++first)
        {
            m_data[first].~T();
        }
    }

    constexpr void destroy()
    {
        LASSERT([&] { return m_capacity ? m_data != nullptr : m_data == nullptr; }());
        clear();
        m_capacity = 0;
        if (m_data)
            free(m_data);
        m_data = nullptr;
    }

    u64 m_capacity{ 0 };
    u64 m_size{ 0 };
    T*  m_data{ nullptr };
};

}