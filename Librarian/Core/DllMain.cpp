/**
 * @file DllMain.cpp
 * @brief Entry point and orchestration for the AutoTheater DLL.
 * * Manages the DLL lifecycle, thread initialization, and global state setup.
 * Uses a secondary initialization thread to prevent DllMain deadlocks.
 */

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

/**
 * @brief Secondary initialization routine.
 * @param lpParam HMODULE handle of the injected DLL.
 * @return 0 on completion.
 * * Performs blocking operations: path resolution, log file creation,
 * MinHook initialization, and spawns the core worker threads.
 */
DWORD WINAPI InitializeLibrarian(LPVOID lpParam)
{
    g_pState = new AppState();

    HMODULE handleModule = (HMODULE)lpParam;
    g_pState->handleModule.store(handleModule);

    char buffer[MAX_PATH];
    GetModuleFileNameA(handleModule, buffer, MAX_PATH);
    PathRemoveFileSpecA(buffer);

    {
        std::lock_guard lock(g_pState->configMutex);
        g_pState->baseDirectory = std::string(buffer);
        g_pState->loggerPath = g_pState->baseDirectory + "\\AutoTheater.txt";
        std::ofstream ofs(g_pState->loggerPath, std::ios::trunc);
    }

    Logger::LogAppend("AutoTheater Initializing...");

    if (MH_Initialize() != MH_OK)
    {
        Logger::LogAppend("ERROR: MH_Initialize failed");
        return 0;
    }

    g_pState->running.store(true);

    g_MainThread = std::thread(MainThread::Run);
    g_LogThread = std::thread(LogThread::Run);
    g_InputThread = std::thread(InputThread::Run);
    g_DirectorThread = std::thread(DirectorThread::Run);

    Logger::LogAppend("All systems started successfully");
    return 0;
}

/** @brief Windows DLL Entry Point. 
 * * - ATTACH: Spawns InitilizeLibrarian to avoid Loader Lock deadlocks.
 * - DETACH: Signals shutdown to all threads and unhooks MinHook.
 */
BOOL APIENTRY DllMain(HMODULE handleModule, DWORD ulReasonForCall, LPVOID lpReserved) {
    switch (ulReasonForCall) {
        case DLL_PROCESS_ATTACH: {
            DisableThreadLibraryCalls(handleModule);

            HANDLE hThread = CreateThread(NULL, 0, InitializeLibrarian, handleModule, 0, NULL);
            if (hThread) CloseHandle(hThread);

            break;
        }

        case DLL_PROCESS_DETACH: {
            /** * @note Important: lpReserved == NULL means that the DLL is being
             * released via FreeLibrary (manually), not because the process finished. 
             */
            if (lpReserved == NULL)
            {
                Logger::LogAppend("[DLL_PROCESS_DETACH]");

                g_pState->running.store(false);

                /** 
                 * @warning We avoid extensive '.join()' here due to Loader Lock.
                 * Instead, we allow some time and use '.detach()' to allow the
                 * OS to clean up if threads get stuck.
                 */
                std::this_thread::sleep_for(150ms);

                MH_Uninitialize();

                if (g_MainThread.joinable()) g_MainThread.detach();
                if (g_LogThread.joinable()) g_LogThread.detach();
                if (g_InputThread.joinable()) g_InputThread.detach();
                if (g_DirectorThread.joinable()) g_DirectorThread.detach();
            }

            break;
        }
    }

    return TRUE;
}