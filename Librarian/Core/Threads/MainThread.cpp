#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Threads/MainThread.h"
#include "Core/Common/GlobalState.h"
#include "Hooks/UserInterface/Present_Hook.h"
#include "Hooks/Lifecycle/GameEngineStart_Hook.h"
#include "Hooks/Lifecycle/EngineInitialize_Hook.h"
#include "Hooks/Lifecycle/DestroySubsystems_Hook.h"
#include <sstream>
#include <chrono>

using namespace std::chrono_literals;

std::thread g_MainThread;

static bool IsHookIntact(void* address)
{
    if (address == nullptr) return false;

    unsigned char firstByte;
    size_t bytesRead;

    if (ReadProcessMemory(GetCurrentProcess(), address, &firstByte, 1, &bytesRead))
    {
        return firstByte == 0xE9;
    }

    return false;
}

static void MainThread::ShutdownAndEject()
{
    Logger::LogAppend("Critical Error: Initiating emergency shutdown.");
    g_pState->running.store(false);
}

static bool TryInstallLifecycleHooks(const char* context)
{
    std::stringstream ss;

    while (g_pState->running.load())
    {
        if (EngineInitialize_Hook::Install(true) &&
             DestroySubsystems_Hook::Install(true) &&
             GameEngineStart_Hook::Install(true)
        ) {
             ss << "All initial hooks installed successfully [" << context << "]";
             Logger::LogAppend(ss.str().c_str());
             ss.str("");
             return true;
         }

        std::this_thread::sleep_for(1s);
    }

    return false;
}

static void UpdatePhase()
{
    if (g_pState->currentPhase.load() == Phase::BuildTimeline)
    {
        g_pState->logGameEvents.store(false);
        g_pState->processedCount.store(0);
        g_pState->currentPhase.store(Phase::ExecuteDirector);
    }
    else
    {
        g_pState->directorInitialized.store(false);
        g_pState->directorHooksReady.store(false);
        g_pState->currentPhase.store(Phase::BuildTimeline);

        std::this_thread::sleep_for(100ms);

        {
            std::lock_guard lock(g_pState->timelineMutex);
            g_pState->timeline.clear();
        }

        {
            std::lock_guard lock(g_pState->directorMutex);
            g_pState->script.clear();
        }

        g_pState->currentCommandIndex.store(0);
        g_pState->lastReplayTime.store(0.0f);
        g_pState->logGameEvents.store(true);
        g_pState->isLastEvent.store(false);
    }

    {
        std::lock_guard lock(g_pState->theaterMutex);
        for (auto& player : g_pState->playerList)
        {
            player.Name.clear();
            player.Tag.clear();
            player.Id = 0;

            player.RawPlayer = RawPlayer{};
            player.Weapons.clear();
        }
    }
}

void MainThread::Run() {
    std::this_thread::sleep_for(5s);

    Logger::LogAppend("=== Main Thread Started ===");

    if (!TryInstallLifecycleHooks("Initial Boot"))
    {
        Logger::LogAppend("ERROR: Timeout waiting for game modules or signatures");
        ShutdownAndEject();
        return;
    }

    Logger::LogAppend("Installing DX11 Present Hook...");
    Present_Hook::Install();

    g_pState->currentPhase.store(Phase::BuildTimeline);
    g_pState->logGameEvents.store(true);

    while (g_pState->running.load())
    {
        if (!IsHookIntact(g_EngineInitialize_Address) ||
            !IsHookIntact(g_DestroySubsystems_Address) ||
            !IsHookIntact(g_GameEngineStart_Address)
        ) {
            Logger::LogAppend("Watchdog: Hook corrupted or memory restored. Rebooting...");
            g_pState->engineStatus.store({ EngineStatus::Destroyed });
        }

        if (g_pState->engineStatus.load() == EngineStatus::Destroyed)
        {
            Logger::LogAppend("Game engine destruction detected, resetting lifecycle...");

            std::this_thread::sleep_for(1s);
            if (!g_pState->running.load()) break;

            if (g_pState->isTheaterMode.load()) UpdatePhase();

            EngineInitialize_Hook::Uninstall();
            DestroySubsystems_Hook::Uninstall();
            GameEngineStart_Hook::Uninstall();

            std::this_thread::sleep_for(1s);
            if (!g_pState->running.load()) break;

            if (!g_pState->running.load())
            {
                Logger::LogAppend("MainThread: Shutdown in progress, skipping re-installation.");
                break;
            }

            if (!TryInstallLifecycleHooks("Engine Reset Cycle")) {
                Logger::LogAppend("ERROR: Failed to re-install hooks after engine reset!");
                ShutdownAndEject();
                return;
            }

            g_pState->engineStatus.store({ EngineStatus::Idle });
            g_pState->isTheaterMode.store(false);
        }

        std::this_thread::sleep_for(1s);
    }

    Logger::LogAppend("=== Main Thread Stopped ===");

    Present_Hook::Uninstall();
    EngineInitialize_Hook::Uninstall();
    DestroySubsystems_Hook::Uninstall();
    GameEngineStart_Hook::Uninstall();

    Logger::LogAppend("=== Cleanup complete. Ejecting mod... ===");

    FreeLibraryAndExitThread(g_pState->handleModule.load(), 0);
}