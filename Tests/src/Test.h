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
// File Name: Test.h
// Date File Created: 8/20/2022
// Author: Matt
// 
// ------------------------------------------------------------------------------
#pragma once

#define TEST_ECS      0
#define TEST_WINDOWS  0
#define TEST_RENDERER 1

#include <thread>
#include <chrono>
#include <string>


#include <Lotus/Core/Types.h>

#include <Windows.h>


class Test
{
public:
    virtual ~Test()         = default;
    virtual bool Init()     = 0;
    virtual void Run()      = 0;
    virtual void Shutdown() = 0;
};

class timer_lt
{
public:
    using clock = std::chrono::high_resolution_clock;
    using time_stamp = std::chrono::steady_clock::time_point;

    constexpr f32 delta_average() const { return m_delta_average * 1e-3f; }

    void begin()
    {
        m_start = clock::now();
    }

    void end()
    {
        auto dt = clock::now() - m_start;
        m_avg_ms += ((f32)std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() - m_avg_ms) / (f32)m_counter;
        ++m_counter;
        m_delta_average = m_avg_ms;

        if(std::chrono::duration_cast<std::chrono::seconds>(clock::now() - m_seconds).count() >= 1)
        {
            OutputDebugStringA("Avg. Frame (ms): ");
            OutputDebugStringA(std::to_string(m_avg_ms).c_str());
            OutputDebugStringA((" " + std::to_string(m_counter)).c_str());
            OutputDebugStringA(" fps");
            OutputDebugStringA("\n");
            m_avg_ms = 0.0f;
            m_counter = 1;
            m_seconds = clock::now();
        }
    }

private:
    f32 m_avg_ms{0.0f};
    f32 m_delta_average{ 16.7f };
    i32 m_counter{1};

    time_stamp m_start;
    time_stamp m_seconds{clock::now()};
};
