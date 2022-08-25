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
// File Name: EngineAPI.cpp
// Date File Created: 08/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "Common.h"
#include <Lotus/Core/Common.h>
#include <Lotus/Components/Script.h>

#ifndef WIN32_LEAN_ANDMEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <atlsafe.h>

using namespace lotus;

namespace
{
HMODULE gameDll = nullptr;

using script_creator_func              = script::detail::script_creator (*)(size_t);
script_creator_func get_script_creator = nullptr;

using script_names_func            = LPSAFEARRAY (*)(void);
script_names_func get_script_names = nullptr;

} // namespace


EDITOR_INTERFACE u32 LoadGameDll(const char* dllPath)
{
    if (gameDll) return FALSE;
    gameDll = LoadLibraryA(dllPath);
    LASSERT(gameDll);

    get_script_names   = (script_names_func) GetProcAddress(gameDll, "get_script_names");
    get_script_creator = (script_creator_func) GetProcAddress(gameDll, "get_script_creator");
    return gameDll && get_script_names && get_script_creator ? TRUE : FALSE;
}

EDITOR_INTERFACE uint32 UnloadGameDll()
{
    if (!gameDll) return FALSE;
    LASSERT(gameDll);
    int res = FreeLibrary(gameDll);
    gameDll = nullptr;
    return TRUE;
}

EDITOR_INTERFACE script::detail::script_creator GetScriptCreator(const char* name)
{
    return gameDll && get_script_creator ? get_script_creator(string_hash()(name)) : nullptr;
}

EDITOR_INTERFACE LPSAFEARRAY GetScriptNames() { return gameDll && get_script_names ? get_script_names() : nullptr; }