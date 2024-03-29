﻿// ------------------------------------------------------------------------------
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
// File Name: IOStream.h
// Date File Created: 02/01/23
// Author: Matt
//
// ------------------------------------------------------------------------------


#pragma once

#include "../Common.h"

template<typename T>
concept primitive_type = std::is_arithmetic_v<T>;

namespace lotus::utl
{

class blob_stream_reader
{
public:
    explicit blob_stream_reader(const u8* buffer) : m_buffer(buffer), m_position(buffer) { assert(buffer); }

    DISABLE_COPY_AND_MOVE(blob_stream_reader);


    template<primitive_type T>
    [[nodiscard]] T read()
    {
        //static_assert(std::is_arithmetic_v<T>, "Template argument should be a primitive type");
        T value = *(T*) m_position;
        m_position += sizeof(T);
        return value;
    }

    void read(u8* buffer, const size_t length)
    {
        memcpy(buffer, m_position, length);
        m_position += length;
    }

    void skip(const size_t offset) { m_position += offset; }

    [[nodiscard]] constexpr const u8* buffer_start() const { return m_buffer; }
    [[nodiscard]] constexpr const u8* position() const { return m_position; }
    [[nodiscard]] constexpr size_t    offset() const { return m_position - m_buffer; }

private:
    const u8* const m_buffer;
    const u8*       m_position;
};

class blob_stream_writer
{
public:
    explicit blob_stream_writer(u8* buffer, const size_t buffer_size) :
        m_buffer(buffer), m_position(buffer), m_buffer_size(buffer_size)
    {
        assert(buffer && buffer_size);
    }

    DISABLE_COPY_AND_MOVE(blob_stream_writer);

    template<primitive_type T>
    void write(T value)
    {
        //static_assert(std::is_arithmetic_v<T>, "Template argument should be a primitive type");
        assert(&m_position[sizeof(T)] <= &m_buffer[m_buffer_size]);
        *(T*) m_position = value;
        m_position += sizeof(T);
    }

    void write(const u8* buffer, const size_t length)
    {
        assert(&m_position[length] <= &m_buffer[m_buffer_size]);
        memcpy(m_position, buffer, length);
        m_position += length;
    }

    void write(const char* buffer, const size_t length)
    {
        assert(&m_position[length] <= &m_buffer[m_buffer_size]);
        memcpy(m_position, buffer, length);
        m_position += length;
    }

    void skip(const size_t offset)
    {
        assert(&m_position[offset] <= &m_buffer[m_buffer_size]);
        m_position += offset;
    }

    [[nodiscard]] constexpr const u8* buffer_start() const { return m_buffer; }
    [[nodiscard]] constexpr const u8* buffer_end() const { return &m_buffer[m_buffer_size]; }
    [[nodiscard]] constexpr const u8* position() const { return m_position; }
    [[nodiscard]] constexpr size_t    offset() const { return m_position - m_buffer; }

private:
    u8* const m_buffer;
    u8*       m_position;
    size_t    m_buffer_size;
};

} // namespace lotus::utl
