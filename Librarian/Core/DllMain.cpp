#include "pch.h"
#include "Core/DllMain.h"
#include "Proxy/ProxyExports.h"
#include "Core/Common/AppCore.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Threads/CoreThread.h"
#include "External/minhook/include/MinHook.h"
#include <fstream>
#include <chrono>
#pragma comment(lib, "shlwapi.lib")

using namespace std::chrono_literals;

AppLoader g_DllInstance;

extern "C" BOOL APIENTRY DllMain(HMODULE handleModule, DWORD ulReasonForCall, LPVOID lpReserved) 
{
    switch (ulReasonForCall) 
    {
        case DLL_PROCESS_ATTACH: 
            return g_DllInstance.OnAttach(handleModule);
            
        case DLL_PROCESS_DETACH: 
            g_DllInstance.OnDetach(lpReserved);
            break;
    }

    return TRUE;
}

BOOL AppLoader::OnAttach(HMODULE hModule)
{
    DisableThreadLibraryCalls(hModule);

    HANDLE hThread = CreateThread(NULL, 0, AppLoader::InitializeLibrarian, hModule, 0, NULL);
    if (hThread) CloseHandle(hThread);

    return TRUE;
}

void AppLoader::OnDetach(LPVOID lpReserved)
{
    this->DeinitializeLibrarian(lpReserved);
}


// Responsible for initializing AutoTheater data and core systems.
DWORD WINAPI AppLoader::InitializeLibrarian(LPVOID lpParam)
{
    // 1. Instantiate the global container.
    g_App = std::make_unique<AppCore>();

    // 3. Store the Module Handle of the main process (MCC-Win64-Shipping.exe).
    HMODULE handleModule = (HMODULE)lpParam;
    g_pState->Lifecycle.SetHandleModule(handleModule);

    // 4. Get the base directory where the main process is running.
    char buffer[MAX_PATH];
    GetModuleFileNameA(handleModule, buffer, MAX_PATH);
    PathRemoveFileSpecA(buffer);

    // 5. Store the base directory and logger path, and creates the logger file.
    g_pSystem->Settings.InitializePaths(buffer);
    std::ofstream ofs(g_pState->Settings.GetLoggerPath(), std::ios::trunc);
    if (g_pState->Settings.ShouldUseAppData())
    {
        g_pSystem->Preferences.LoadPreferences();
    }

    // 6. Attempt to initialize MinHook.
    if (MH_Initialize() != MH_OK)
    {
        g_pUtil->Log.Append("[DllMain] ERROR: MH_Initialize failed.");
        return 0;
    }

    // 7. Signal that the application started running.
    g_pState->Lifecycle.SetRunning(true);

    // 8. Initialize main worker threads.

    // Manages the hooks lifecycle and main application state updates.
    g_DllInstance.m_MainThread = std::thread(&MainThread::Run, &g_pThread->Main);

    // Process and writes the logs for captured GameEvents.
    g_DllInstance.m_LogThread = std::thread(&LogThread::Run, &g_pThread->Log);

    // Handles both manual user input and automated input injection into the game engine.
    g_DllInstance.m_InputThread = std::thread(&InputThread::Run, &g_pThread->Input);

    // Manages the director logic, including initialization and update.
    g_DllInstance.m_DirectorThread = std::thread(&DirectorThread::Run, &g_pThread->Director);

    // Acts as the core of the capture system.
    g_DllInstance.m_CaptureThread = std::thread(&CaptureThread::Run, &g_pThread->Capture);

    g_pUtil->Log.Append("[DllMain] INFO: AutoTheater Initialized.");
    return 0;
}

void AppLoader::DeinitializeLibrarian(LPVOID lpReserved)
{
    // 1. Check if AutoTheater called FreeLibrary directly.
    if (lpReserved == NULL)
    {
        g_pUtil->Log.Append("[DllMain] INFO: Deinitializing AutoTheater.");

        // 2. Signal that the application has stopped running.
        g_pState->Lifecycle.SetRunning(false);

        std::this_thread::sleep_for(100ms);

        // 3. Deinitialize MinHook.
        MH_Uninitialize();

        // 4. If workers are active, release them.
        if (m_MainThread.joinable()) m_MainThread.detach();
        if (m_LogThread.joinable()) m_LogThread.detach();
        if (m_InputThread.joinable()) m_InputThread.detach();
        if (m_DirectorThread.joinable()) m_DirectorThread.detach();
        if (m_CaptureThread.joinable()) m_CaptureThread.detach();

        // 5. Destroy the application.
        g_App.reset();

        g_pUtil->Log.Append("[DllMain] INFO: AutoTheater Deinitialized.");
    }
}