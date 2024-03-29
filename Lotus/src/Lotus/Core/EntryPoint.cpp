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
// File Name: EntryPoint.cpp
// Date File Created: 8/23/2022
// Author: Matt
//
// ------------------------------------------------------------------------------

#include "Common.h"

#include <filesystem>

#ifdef _WIN64

    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif

    #include <Windows.h>
    #include <crtdbg.h>

namespace
{
std::filesystem::path set_current_directory_to_exe_path()
{
    wchar_t   path[MAX_PATH];
    const u32 length = GetModuleFileName(nullptr, &path[0], MAX_PATH);
    if (!length || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        return {};

    const std::filesystem::path p = path;
    std::filesystem::current_path(p.parent_path());
    return std::filesystem::current_path();
}
} // anonymous namespace

    #ifndef L_EDITOR
extern bool engine_initialize();
extern void engine_update();
extern void engine_shutdown();

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
        #if L_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        #endif
    set_current_directory_to_exe_path();
    if (engine_initialize())
    {
        MSG  msg;
        bool running = true;

        while (running)
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                running &= (msg.message != WM_QUIT);
            }

            engine_update();
        }
    }

    engine_shutdown();
    return 0;
}

    #endif


#else
    #error Lotus currently only supports Windows x64
#endif