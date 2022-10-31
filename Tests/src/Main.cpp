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
// File Name: Main.cpp
// Date File Created: 08/20/2022
// Author: Matt
//
// ------------------------------------------------------------------------------

#pragma comment(lib, "Lotus.lib")

#define TEST_ECS      0
#define TEST_WINDOWS  0
#define TEST_RENDERER 1

#if TEST_ECS
    #include "EntityComponentSystemTest.h"
#elif TEST_WINDOWS
    #include "WindowTest.h"
#elif TEST_RENDERER
    #include "TestRenderer.h"
#else
    #error A test has not been enabled
#endif

#ifdef _WIN64
    #include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    #if L_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif
    EngineTest engineTest;
    if (engineTest.Init())
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

            engineTest.Run();
        }
    }

    engineTest.Shutdown();
    return 0;
}

#endif