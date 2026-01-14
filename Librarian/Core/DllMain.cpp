#include "pch.h"
#include "Utils/Logger.h"
#include "Core/DllMain.h"
#include "Core/Threads/MainThread.h"
#include "Core/Threads/LogThread.h"
#include "Core/Threads/InputThread.h"
#include "Core/Threads/DirectorThread.h"
#include "Proxy/ProxyExports.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

LibrarianPhase g_CurrentPhase;
uintptr_t g_BaseModuleAddress;
std::atomic<bool> g_Running;
std::string g_BaseDirectory;
HMODULE g_HandleModule;

BOOL APIENTRY DllMain(HMODULE handleModule, DWORD ulReasonForCall, LPVOID lpReserved) {
    switch (ulReasonForCall) {
    case DLL_PROCESS_ATTACH: {
        DisableThreadLibraryCalls(handleModule);
        g_HandleModule = handleModule;

        char buffer[MAX_PATH];
        GetModuleFileNameA(g_HandleModule, buffer, MAX_PATH);
        PathRemoveFileSpecA(buffer);
        g_BaseDirectory = std::string(buffer);

        g_LoggerPath = g_BaseDirectory + "\\AutoTheater.txt";
        std::ofstream ofs(g_LoggerPath, std::ios::trunc);
        Logger::LogAppend("AutoTheater Proxy Loaded Successfully");

        if (MH_Initialize() != MH_OK) {
            Logger::LogAppend("ERROR: MH_Initialize failed in DllMain");
            return FALSE;
        }

        Logger::LogAppend("MinHook initialized successfully");

        g_Running.store(true);

        g_CurrentPhase = LibrarianPhase::Start;
        Logger::LogAppend("=== Current Phase: Start ===");

        g_MainThread = std::thread(MainThread::Run);
        g_LogThread = std::thread(LogThread::Run);
        g_InputThread = std::thread(InputThread::Run);
        g_DirectorThread = std::thread(DirectorThread::Run);

        break;
    }

    case DLL_PROCESS_DETACH: {
        Logger::LogAppend("=== DLL_PROCESS_DETACH ===");

        g_Running.store(false); 

        if (lpReserved == NULL)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

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