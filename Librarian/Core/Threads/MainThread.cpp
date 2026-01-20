#include "pch.h"
#include "LogThread.h"
#include "Core/DllMain.h"
#include "Core/Systems/Director.h"
#include "Core/Threads/MainThread.h"
#include "Hooks/Lifecycle/GameEngineStart_Hook.h"
#include "Hooks/Lifecycle/EngineInitialize_Hook.h"
#include "Hooks/Lifecycle/DestroySubsystems_Hook.h"
#include "Utils/Logger.h"

std::thread g_MainThread;

static void ShutdownAndEject()
{
    g_Running.store(false);

    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        FreeLibraryAndExitThread(g_HandleModule, 0);
    }).detach();
}

bool TryInstallLifecycleHooks(const char* context)
{
    const int MAX_ATTEMPTS = 120;
    std::stringstream ss;
    int attempts = 0;

    while (g_Running.load() && attempts < MAX_ATTEMPTS)
    {
        if (GetModuleHandle(L"haloreach.dll") != nullptr)
        {
            if (EngineInitialize_Hook::Install() &&
                DestroySubsystems_Hook::Install() &&
                GameEngineStart_Hook::Install()
                ) {
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
}

void UpdatePhase()
{
    if (g_CurrentPhase == LibrarianPhase::BuildTimeline)
    {
        g_LogGameEvents = false;
        g_CurrentPhase = LibrarianPhase::ExecuteDirector;
    }
    else
    {
        g_DirectorInitialized.store(false);
        g_EngineHooksReady.store(false);
        g_CurrentPhase = LibrarianPhase::BuildTimeline;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        {
            std::lock_guard<std::mutex> lock(g_TimelineMutex);
            g_Timeline.clear();
        }

        {
            std::lock_guard<std::mutex> lock(g_ScriptMutex);
            g_Script.clear();
            g_CurrentCommandIndex = 0;
        }

        g_LogGameEvents = true;
        g_IsLastEvent = false;
    }
}

void MainThread::Run() {
    std::this_thread::sleep_for(std::chrono::seconds(5));

    Logger::LogAppend("=== Main Thread Started ===");

    if (!TryInstallLifecycleHooks("Initial Boot"))
    {
        Logger::LogAppend("ERROR: Timeout waiting for game modules or signatures");
        ShutdownAndEject();
        return;
    }

    g_CurrentPhase = LibrarianPhase::BuildTimeline;
    g_LogGameEvents = true;

    while (g_Running.load())
    {
        if (g_GameEngineDestroyed)
        {
            Logger::LogAppend("Game engine destruction detected, resetting lifecycle...");
            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (g_IsTheaterMode) UpdatePhase();

            EngineInitialize_Hook::Uninstall();
            DestroySubsystems_Hook::Uninstall();
            GameEngineStart_Hook::Uninstall();

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
    GameEngineStart_Hook::Uninstall();

    Logger::LogAppend("=== Main Thread Stopped ===");
}