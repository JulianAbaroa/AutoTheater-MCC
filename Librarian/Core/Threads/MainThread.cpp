#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Threads/MainThread.h"
#include "Core/Common/GlobalState.h"
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

static void ShutdownAndEject()
{
    g_State.running.store(false);

    std::thread([=]() {
        std::this_thread::sleep_for(200ms);
        FreeLibraryAndExitThread(g_State.handleModule, 0);
    }).detach();
}

static bool TryInstallLifecycleHooks(const char* context)
{
    std::stringstream ss;

    while (g_State.running.load())
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
    if (g_State.currentPhase.load() == Phase::BuildTimeline)
    {
        g_State.logGameEvents.store(false);
        g_State.processedCount.store(0);
        g_State.currentPhase.store(Phase::ExecuteDirector);
    }
    else
    {
        g_State.directorInitialized.store(false);
        g_State.engineHooksReady.store(false);
        g_State.currentPhase.store(Phase::BuildTimeline);

        std::this_thread::sleep_for(100ms);

        {
            std::lock_guard lock(g_State.timelineMutex);
            g_State.timeline.clear();
        }

        {
            std::lock_guard lock(g_State.directorMutex);
            g_State.script.clear();
        }

        g_State.currentCommandIndex.store(0);
        g_State.logGameEvents.store(true);
        g_State.isLastEvent.store(false);
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

    g_State.currentPhase.store(Phase::BuildTimeline);
    g_State.logGameEvents.store(true);

    while (g_State.running.load())
    {
        if (!IsHookIntact(g_EngineInitialize_Address) ||
            !IsHookIntact(g_DestroySubsystems_Address) ||
            !IsHookIntact(g_GameEngineStart_Address)
        ) {
            Logger::LogAppend("Watchdog: Hook corrupted or memory restored. Rebooting...");
            g_State.gameEngineDestroyed.store(true);
        }

        if (g_State.gameEngineDestroyed.load())
        {
            Logger::LogAppend("Game engine destruction detected, resetting lifecycle...");
            std::this_thread::sleep_for(1s);

            if (g_State.isTheaterMode.load()) UpdatePhase();

            EngineInitialize_Hook::Uninstall();
            DestroySubsystems_Hook::Uninstall();
            GameEngineStart_Hook::Uninstall();

            std::this_thread::sleep_for(1s);

            if (!TryInstallLifecycleHooks("Engine Reset Cycle")) {
                Logger::LogAppend("ERROR: Failed to re-install hooks after engine reset!");
                ShutdownAndEject();
                return;
            }

            g_State.gameEngineDestroyed.store(false);
        }

        std::this_thread::sleep_for(1s);
    }

    EngineInitialize_Hook::Uninstall();
    DestroySubsystems_Hook::Uninstall();
    GameEngineStart_Hook::Uninstall();

    Logger::LogAppend("=== Main Thread Stopped ===");
}