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

static void ShutdownAndEject()
{
    g_Running.store(false);

    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        FreeLibraryAndExitThread(g_HandleModule, 0);
    }).detach();
}

void MainThread::Run() {
    Logger::LogAppend("=== Main Thread Started ===");

    const int MAX_ATTEMPTS = 120;
    auto TryInstallLifecycleHooks = [&](const char* context) -> bool {
        std::stringstream ss;
        int attempts = 0;

        while (g_Running.load() && attempts < MAX_ATTEMPTS)
        {
            if (GetModuleHandle(L"haloreach.dll") != nullptr)
            {
                if (EngineInitialize_Hook::Install() && DestroySubsystems_Hook::Install())
                {
                    ss << "All initial hooks installed successfully [" << context << "]";
                    Logger::LogAppend(ss.str().c_str());
                    ss.str("");
                    return true;
                }
            }

            attempts++;
            ss << "Waiting for hooks... (Attempt " << std::to_string(attempts) << ") [" << context << "]";
            Logger::LogAppend(ss.str().c_str());
            ss.str("");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        return false;
    };

    if (!TryInstallLifecycleHooks("Initial Boot"))
    {
        Logger::LogAppend("ERROR: Timeout waiting for game modules or signatures");
        ShutdownAndEject();
        return;
    }

    g_BaseModuleAddress = (uintptr_t)Main::GetHaloReachModuleBaseAddress();
    g_CurrentPhase = LibrarianPhase::BuildTimeline;
    Logger::LogAppend("=== Current Phase: BuildTimeline ===");

    while (g_Running.load())
    {
        if (g_GameEngineDestroyed)
        {
            Logger::LogAppend("Game engine destruction detected, resetting lifecycle...");
            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (g_CurrentPhase == LibrarianPhase::BuildTimeline)
            {
                g_CurrentPhase = LibrarianPhase::ExecuteDirector;
                Logger::LogAppend("=== Current Phase: ExecuteDirector ===");
            }
            else
            {
                g_CurrentPhase = LibrarianPhase::BuildTimeline;
                Logger::LogAppend("=== Current Phase: BuildTimeline ===");

                g_DirectorInitialized = false;
                g_IsLastEvent = false;
                g_Timeline.clear();
                g_Script.clear();
            }

            EngineInitialize_Hook::Uninstall();
            DestroySubsystems_Hook::Uninstall();

            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (!TryInstallLifecycleHooks("Engine Reset Cycle")) {
                Logger::LogAppend("ERROR: Failed to re-install hooks after engine reset!");
                ShutdownAndEject();
                return;
            }

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