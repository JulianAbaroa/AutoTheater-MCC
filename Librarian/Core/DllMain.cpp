#include "pch.h"
#include "Core/DllMain.h"
#include "Proxy/ProxyExports.h"
#include "Core/Common/AppCore.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Persistence/SettingsSystem.h"
#include "Core/Systems/Infrastructure/Persistence/PreferencesSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Domain/MainThread.h"
#include "Core/Threads/Domain/DirectorThread.h"
#include "Core/Threads/Infrastructure/InputThread.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"
#include "External/minhook/include/MinHook.h"
#include <fstream>
#include <chrono>
#pragma comment(lib, "shlwapi.lib")

// TODO: It crashes when alt+f4 on director mode.
// TODO: Add feedback to capture tab when recording is active.

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
    g_App = std::make_unique<AppCore>();

    HMODULE handleModule = (HMODULE)lpParam;
    g_pState->Infrastructure->Lifecycle->SetHandleModule(handleModule);

    char buffer[MAX_PATH];
    GetModuleFileNameA(handleModule, buffer, MAX_PATH);
    PathRemoveFileSpecA(buffer);

    g_pSystem->Infrastructure->Settings->InitializePaths(buffer);
    std::ofstream ofs(g_pState->Infrastructure->Settings->GetLoggerPath(), std::ios::trunc);
    if (g_pState->Infrastructure->Settings->ShouldUseAppData())
    {
        g_pSystem->Infrastructure->Preferences->LoadPreferences();
    }

    if (MH_Initialize() != MH_OK)
    {
        g_pSystem->Debug->Log("[DllMain] ERROR: MH_Initialize failed.");
        return 0;
    }

    g_pState->Infrastructure->Lifecycle->SetRunning(true);

    g_DllInstance.m_MainThread = std::thread(&MainThread::Run, g_pThread->Main.get());
    g_DllInstance.m_InputThread = std::thread(&InputThread::Run, g_pThread->Input.get());
    g_DllInstance.m_DirectorThread = std::thread(&DirectorThread::Run, g_pThread->Director.get());
    g_DllInstance.m_CaptureThread = std::thread(&CaptureThread::Run, g_pThread->Capture.get());

    g_pSystem->Debug->Log("[DllMain] INFO: AutoTheater Initialized.");
    return 0;
}

void AppLoader::DeinitializeLibrarian(LPVOID lpReserved)
{
    if (lpReserved == NULL)
    {
        g_pSystem->Debug->Log("[DllMain] INFO: Deinitializing AutoTheater.");

        g_pState->Infrastructure->Lifecycle->SetRunning(false);

        std::this_thread::sleep_for(100ms);

        MH_Uninitialize();

        if (m_MainThread.joinable()) m_MainThread.detach();
        if (m_InputThread.joinable()) m_InputThread.detach();
        if (m_DirectorThread.joinable()) m_DirectorThread.detach();
        if (m_CaptureThread.joinable()) m_CaptureThread.detach();

        g_App.reset();

        g_pSystem->Debug->Log("[DllMain] INFO: AutoTheater Deinitialized.");
    }
}