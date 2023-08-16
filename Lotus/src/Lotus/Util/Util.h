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
// File Name: Util.h
// Date File Created: 08/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#define USE_STL_VECTOR 0
#define USE_STL_DEQUE  1

#if USE_STL_VECTOR
    #include <vector>
    #include <algorithm>
namespace lotus::utl
{
template<typename T>
using vector = std::vector<T>;

template<typename T>
void erase_unordered(T& vec, size_t index)
{
    if (vec.size() > 1)
    {
        std::iter_swap(vec.begin() + index, vec.end() - 1);
        vec.pop_back();
    } else
    {
        vec.clear();
    }
}

} // namespace lotus::utl

#else
    #include "Vector.h"

namespace lotus::utl
{
template<typename T>
void erase_unordered(T& v, size_t index)
{
    v.erase_unordered(index);
}
} // namespace lotus::utl

#endif

#if USE_STL_DEQUE
    #include <deque>
namespace lotus::utl
{
template<typename T>
using deque = std::deque<T>;
} // namespace lotus::utl
#endif


#include "FreeList.h"

namespace lotus::utl
{
inline std::wstring to_wstring(const char* c)
{
    std::string s{ c };
    return { s.begin(), s.end() };
}
} // namespace lotus::utl