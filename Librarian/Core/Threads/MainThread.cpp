#include "pch.h"
#include "Core/DllMain.h"
#include "Utils/Logger.h"
#include "Core/Systems/Director.h"
#include "Core/Threads/MainThread.h"
#include "Hooks/Lifecycle/EngineInitialize_Hook.h"
#include "Hooks/Lifecycle/DestroySubsystems_Hook.h"

#include "Hooks/MovReader/BlamOpenFile_Hook.h"
#include "Hooks/MovReader/FilmInitializeState_Hook.h"
#include "Hooks/Data/SpectatorHandleInput_Hook.h"
#include "Hooks/Data/UpdateTelemetryTimer_Hook.h"
#include "Hooks/Data/UIBuildDynamicMessage_Hook.h"

std::thread g_MainThread;

void MainThread::Run() {
    Logger::LogAppend("=== Main Thread Started ===");

    int attempts = 0;
    const int MAX_ATTEMPTS = 120;

    while (g_Running.load() && attempts < MAX_ATTEMPTS)
    {
        if (GetModuleHandle(L"haloreach.dll") != nullptr)
        {

            if (EngineInitialize_Hook::Install() && DestroySubsystems_Hook::Install())
            {
                Logger::LogAppend("All initial hooks installed successfully");
                break;
            }
        }

        attempts++;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (attempts > MAX_ATTEMPTS)
    {
        Logger::LogAppend("ERROR: Timeout waiting for game modules or signatures");
    }

    g_BaseModuleAddress = (uintptr_t)Main::GetHaloReachModuleBaseAddress();
    g_CurrentPhase = LibrarianPhase::BuildTimeline;
    Logger::LogAppend("=== Current Phase: BuildTimeline ===");

    while (g_Running.load()) {
        if (g_GameEngineDestroyed)
        {
            if (g_CurrentPhase == LibrarianPhase::BuildTimeline)
            {
                g_CurrentPhase = LibrarianPhase::ExecuteDirector;
                Logger::LogAppend("=== Current Phase: ExecuteDirector ===");
            }
            else if (g_CurrentPhase == LibrarianPhase::ExecuteDirector)
            {
                g_CurrentPhase = LibrarianPhase::BuildTimeline;
                Logger::LogAppend("=== Current Phase: BuildTimeline ===");

                g_DirectorInitialized = false;
                g_Timeline.clear();
                g_Script.clear();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            EngineInitialize_Hook::Uninstall();
            DestroySubsystems_Hook::Uninstall();

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            EngineInitialize_Hook::Install();
            DestroySubsystems_Hook::Install();

            g_GameEngineDestroyed = false;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    g_CurrentPhase = LibrarianPhase::End;
    Logger::LogAppend("=== Current Phase: End ===");

    EngineInitialize_Hook::Uninstall();
    DestroySubsystems_Hook::Uninstall();

    BlamOpenFile_Hook::Uninstall();
    FilmInitializeState_Hook::Uninstall();
    UpdateTelemetryTimer_Hook::Uninstall();
    UIBuildDynamicMessage_Hook::Uninstall();
    SpectatorHandleInput_Hook::Uninstall();

    Logger::LogAppend("=== Main Thread Stopped ===");
}