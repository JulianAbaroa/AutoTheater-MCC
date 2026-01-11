#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Proxy/ProxyExports.h"
#include "Core/Threads/MainThread.h"
#include "Core/Threads/InputThread.h"
#include "Core/Threads/TheaterThread.h"
#include "Core/Threads/DirectorThread.h"
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

// Project name: AutoTheater for MCC (Halo Reach)

LibrarianPhase g_CurrentPhase = LibrarianPhase::Start;
std::atomic<bool> g_Running{ false };
uintptr_t g_BaseModuleAddress = 0;
HMODULE g_HandleModule = nullptr;
std::string g_BaseDirectory = "";
DWORD g_GamePID = 0;

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

        g_GamePID = GetCurrentProcessId();

        std::stringstream ss;
        ss << "AutoTheater PID: " << std::to_string(g_GamePID);
        Logger::LogAppend(ss.str().c_str());

        Logger::LogAppend("=== Current Phase: Start ===");

        g_MainThread = std::thread(MainThread::Run);

        std::this_thread::sleep_for(std::chrono::seconds(2));

        g_InputThread = std::thread(InputThread::Run);
        g_TheaterThread = std::thread(TheaterThread::Run);
        g_DirectorThread = std::thread(DirectorThread::Run);

        break;
    }

    case DLL_PROCESS_DETACH: {
        Logger::LogAppend("=== DLL_PROCESS_DETACH ===");

        g_Running.store(false); 

        if (lpReserved == NULL)
        {
            if (g_MainThread.joinable()) g_MainThread.detach();
            if (g_InputThread.joinable()) g_InputThread.detach();
            if (g_TheaterThread.joinable()) g_TheaterThread.detach();
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