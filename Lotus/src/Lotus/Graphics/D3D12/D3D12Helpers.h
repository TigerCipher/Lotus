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
// File Name: D3D12Helpers.h
// Date File Created: 01/18/2023
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once

#include "D3D12Common.h"

namespace lotus::graphics::d3d12::d3dx
{

constexpr struct
{
    const D3D12_HEAP_PROPERTIES default_heap{ D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                                                      D3D12_MEMORY_POOL_UNKNOWN, 0, 0 };
} heap_properties;

}