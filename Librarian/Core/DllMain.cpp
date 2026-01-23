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

BOOL APIENTRY DllMain(HMODULE handleModule, DWORD ulReasonForCall, LPVOID lpReserved) {
    switch (ulReasonForCall) {
    case DLL_PROCESS_ATTACH: {
        DisableThreadLibraryCalls(handleModule);
        g_State.handleModule.store(handleModule);

        char buffer[MAX_PATH];
        GetModuleFileNameA(handleModule, buffer, MAX_PATH);
        PathRemoveFileSpecA(buffer);

        {
            std::lock_guard lock(g_State.configMutex);

            g_State.baseDirectory = std::string(buffer);
            g_State.loggerPath = g_State.baseDirectory + "\\AutoTheater.txt";
            std::ofstream ofs(g_State.loggerPath, std::ios::trunc);
        }

        Logger::LogAppend("AutoTheater Proxy Loaded Successfully");

        if (MH_Initialize() != MH_OK) {
            Logger::LogAppend("ERROR: MH_Initialize failed in DllMain");
            return FALSE;
        }

        Logger::LogAppend("MinHook initialized successfully");

        g_State.running.store(true);

        g_MainThread = std::thread(MainThread::Run);
        g_LogThread = std::thread(LogThread::Run);
        g_InputThread = std::thread(InputThread::Run);
        g_DirectorThread = std::thread(DirectorThread::Run);

        break;
    }

    case DLL_PROCESS_DETACH: {
        Logger::LogAppend("=== DLL_PROCESS_DETACH ===");

        g_State.running.store(false);

        if (lpReserved == NULL)
        {
            std::this_thread::sleep_for(100ms);

            if (g_MainThread.joinable()) g_MainThread.detach();
            if (g_LogThread.joinable()) g_LogThread.detach();
            if (g_InputThread.joinable()) g_InputThread.detach();
            if (g_DirectorThread.joinable()) g_DirectorThread.detach();
        }

        MH_STATUS status = MH_Uninitialize();

        break;
    }

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}