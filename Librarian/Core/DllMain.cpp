#include "pch.h"
#include "Utils/Logger.h"
#include "Proxy/ProxyExports.h"
#include "Core/Common/GlobalState.h"
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
    // 1. Instantiate the global AppState.
    g_pState = new AppState();

    // 2. Store the Module Handle of the main process (MCC-Win64-Shipping.exe).
    HMODULE handleModule = (HMODULE)lpParam;
    g_pState->HandleModule.store(handleModule);

    // 3. Get the base directory where the main process is running.
    char buffer[MAX_PATH];
    GetModuleFileNameA(handleModule, buffer, MAX_PATH);
    PathRemoveFileSpecA(buffer);

    // 4. Store the base directory and logger path, and creates the logger file.
    {
        std::lock_guard<std::mutex> lock(g_pState->ConfigMutex);
        g_pState->BaseDirectory = std::string(buffer);
        g_pState->LoggerPath = g_pState->BaseDirectory + "\\AutoTheater.txt";
        std::ofstream ofs(g_pState->LoggerPath, std::ios::trunc);
    }

    Logger::LogAppend("AutoTheater Initializing.");

    // 5. Attempt to initialize MinHook.
    if (MH_Initialize() != MH_OK)
    {
        Logger::LogAppend("ERROR: MH_Initialize failed.");
        return 0;
    }

    // 6. Signal that the application started running.
    g_pState->Running.store(true);

    // 7. Initialize main worker threads.
    
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
        g_pState->Running.store(false);

        std::this_thread::sleep_for(100ms);

        // 3. Deinitialize MinHook.
        MH_Uninitialize();

        // 4. If workers are active, release them.
        if (g_MainThread.joinable()) g_MainThread.detach();
        if (g_LogThread.joinable()) g_LogThread.detach();
        if (g_InputThread.joinable()) g_InputThread.detach();
        if (g_DirectorThread.joinable()) g_DirectorThread.detach();

        Logger::LogAppend("Cleanup finished.");

        // 5. Delete the AppState instance.
        delete g_pState;
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