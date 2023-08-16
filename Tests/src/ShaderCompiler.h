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
// File Name: ShaderCompiler.h
// Date File Created: 01/21/2023
// Author: Matt
//
// ------------------------------------------------------------------------------
#pragma once
#include <Lotus/Common.h>

struct shader_type
{
    enum type : u32
    {
        vertex = 0,
        hull,
        domain,
        geometry,
        pixel,
        compute,
        amplification,
        mesh,

        count
    };
};

struct shader_file_info
{
    const char*                                        file_name;
    const char*                                        function;
    shader_type::type type;
};

scope<u8[]> compile_shader(shader_file_info info, const char* file_path, lotus::utl::vector<std::wstring>& extra_args);
bool compile_shaders();