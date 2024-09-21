// ------------------------------------------------------------------------------
//
// Lotus
// Copyright 2024 Matthew Rogers
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// File Name: Input.h
// Date File Created: 09/21/2024
// Author: Matt
//
// ------------------------------------------------------------------------------


#pragma once

#include "Common.h"
#include "API/Input.h"

namespace lotus::input
{
void set(input_source::type type, input_code::code code, vec3 value);
void get(input_source::type type, input_code::code code, input_value& value);
}