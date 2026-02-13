#include "pch.h"
#include "Utils/Logger.h"
#include "Proxy/ProxyExports.h"
#include "Core/Common/AppCore.h"
#include "Core/Threads/MainThread.h"
#include "Core/Threads/LogThread.h"
#include "Core/Threads/InputThread.h"
#include "Core/Threads/DirectorThread.h"
#include "External/minhook/include/MinHook.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <fstream>
#include <chrono>

using namespace std::chrono_literals;

// Responsible for initializing AutoTheater data and core systems.
static DWORD WINAPI InitializeLibrarian(LPVOID lpParam)
{
    // 1. Instantiate the global container.
    g_App = std::make_unique<AppCore>();

    // 2. Configure convenience pointers.
    g_pState = g_App->State.get();
    g_pSystem = g_App->System.get();

    // 3. Store the Module Handle of the main process (MCC-Win64-Shipping.exe).
    HMODULE handleModule = (HMODULE)lpParam;
    g_pState->Lifecycle.SetHandleModule(handleModule);

    // 4. Get the base directory where the main process is running.
    char buffer[MAX_PATH];
    GetModuleFileNameA(handleModule, buffer, MAX_PATH);
    PathRemoveFileSpecA(buffer);

    // 5. Store the base directory and logger path, and creates the logger file.
    g_pState->Settings.SetBaseDirectory(std::string(buffer));
    g_pState->Settings.SetLoggerPath(g_pState->Settings.GetBaseDirectory() + "\\AutoTheater.txt");
    std::ofstream ofs(g_pState->Settings.GetLoggerPath(), std::ios::trunc);

    Logger::LogAppend("AutoTheater Initializing.");

    // 6. Attempt to initialize MinHook.
    if (MH_Initialize() != MH_OK)
    {
        Logger::LogAppend("ERROR: MH_Initialize failed.");
        return 0;
    }

    // 7. Signal that the application started running.
    g_pState->Lifecycle.SetRunning(true);

    // 8. Initialize main worker threads.
    
    // Manages the hooks lifecycle and main application state updates.
    g_MainThread = std::thread(MainThread::Run);

    // Process and writes the logs for captured GameEvents.
    g_LogThread = std::thread(LogThread::Run);

    // Handles both manual user input and automated input injection into the game engine.
    g_InputThread = std::thread(InputThread::Run);

    // Manages the director logic, including initialization and update.
    g_DirectorThread = std::thread(DirectorThread::Run);

    Logger::LogAppend("AutoTheater Initialized.");
    return 0;
}

static void DeinitializeLibrarian(LPVOID lpReserved)
{
    // 1. Check if AutoTheater called FreeLibrary directly.
    if (lpReserved == NULL)
    {
        Logger::LogAppend("[DLL_PROCESS_DETACH]");

        // 2. Signal that the application has stopped running.
        g_pState->Lifecycle.SetRunning(false);

        std::this_thread::sleep_for(100ms);

        // 3. Deinitialize MinHook.
        MH_Uninitialize();

        // 4. If workers are active, release them.
        if (g_MainThread.joinable()) g_MainThread.detach();
        if (g_LogThread.joinable()) g_LogThread.detach();
        if (g_InputThread.joinable()) g_InputThread.detach();
        if (g_DirectorThread.joinable()) g_DirectorThread.detach();

        // 5. Destroy the application.
        g_App.reset();

        g_pState = nullptr;
        g_pSystem = nullptr;

        Logger::LogAppend("Cleanup finished.");
    }
}

// Manages the AutoTheater lifecycle.
BOOL APIENTRY DllMain(HMODULE handleModule, DWORD ulReasonForCall, LPVOID lpReserved) {
    switch (ulReasonForCall) {
        case DLL_PROCESS_ATTACH: {
            // 1. Notify Windows not to call DllMain for subsequent thread events.
            DisableThreadLibraryCalls(handleModule);

            // 2. Create a thread to handle AutoTheater initialization.
            HANDLE hThread = CreateThread(NULL, 0, InitializeLibrarian, handleModule, 0, NULL);
            if (hThread) CloseHandle(hThread);

            break;
        }

        case DLL_PROCESS_DETACH: {
            // 3. Call the AutoTheater deinitialization routine.
            DeinitializeLibrarian(lpReserved);

            break;
        }
    }

    return TRUE;
}