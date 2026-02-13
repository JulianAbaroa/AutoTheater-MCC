#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/ThreadUtils.h"
#include "Core/Threads/MainThread.h"
#include "Core/Common/AppCore.h"
#include "Core/Common/PersistenceManager.h"
#include "Core/Hooks/Lifecycle/GameEngineStart_Hook.h"
#include "Core/Hooks/Lifecycle/EngineInitialize_Hook.h"
#include "Core/Hooks/Lifecycle/DestroySubsystems_Hook.h"
#include "Core/Hooks/UserInterface/ResizeBuffers_Hook.h"
#include "Core/Hooks/UserInterface/Present_Hook.h"
#include <sstream>
#include <chrono>

using namespace std::chrono_literals;

std::thread g_MainThread;

static bool IsHookIntact(void* address)
{
    if (address == nullptr) return false;

    unsigned char firstByte{};
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
    g_pState->Lifecycle.SetRunning(false);
}

static bool TryInstallLifecycleHooks(const char* context)
{
    std::stringstream ss;

    while (g_pState->Lifecycle.IsRunning())
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

        ThreadUtils::WaitOrExit(1000ms);
    }

    return false;
}

void MainThread::UpdateToPhase(AutoTheaterPhase targetPhase)
{
    if (targetPhase == g_pState->Lifecycle.GetCurrentPhase()) return;

    // Reset timeline
    if (targetPhase == AutoTheaterPhase::Timeline)
    {
        g_pState->Timeline.SetLoggingActive(true);
        g_pSystem->Timeline.SetLastEventReached(false);

        g_pState->Timeline.ClearTimeline();
    }
    else if (targetPhase == AutoTheaterPhase::Director)
    {
        g_pState->Timeline.SetLoggingActive(false);
        g_pSystem->Timeline.SetLastEventReached(true);
    }
    g_pSystem->Timeline.SetLoggedEventsCount(0);

    // Reset theater
    g_pState->Theater.ResetPlayerList();

    // Reset director
    g_pState->Director.SetInitialized(false);
    g_pState->Director.SetHooksReady(false);
    g_pSystem->Director.SetCurrentCommandIndex(0);
    g_pSystem->Director.SetLastReplayTime(0.0f);
    g_pState->Director.ClearScript();

    // Reset input
    g_pState->Input.Reset();

    // Update phase
    g_pState->Lifecycle.SetCurrentPhase(targetPhase);
}

void MainThread::Run() {
    ThreadUtils::WaitOrExit(5000ms);

    Logger::LogAppend("=== Main Thread Started ===");

    if (!TryInstallLifecycleHooks("Initial Boot"))
    {
        Logger::LogAppend("ERROR: Timeout waiting for game modules or signatures");
        ShutdownAndEject();
        return;
    }

    Logger::LogAppend("Installing DX11 Present Hook...");
    Present_Hook::Install();
    ResizeBuffers_Hook::Install();

    g_pState->Lifecycle.SetCurrentPhase(AutoTheaterPhase::Timeline);
    g_pState->Timeline.SetLoggingActive(true);

    g_pSystem->Settings.InitializePaths();
    g_pSystem->EventRegistry.LoadEventRegistry();

    Logger::LogAppend("Settings and EventRegistry loaded");

    while (g_pState->Lifecycle.IsRunning())
    {
        if (!IsHookIntact(g_EngineInitialize_Address) ||
            !IsHookIntact(g_DestroySubsystems_Address) ||
            !IsHookIntact(g_GameEngineStart_Address)
        ) {
            if (g_pState->Lifecycle.IsRunning())
            {
                Logger::LogAppend("Watchdog: Hook corrupted or memory restored. Rebooting...");
                g_pState->Lifecycle.SetEngineStatus({ EngineStatus::Destroyed });
            }
        }

        if (g_pState->Lifecycle.GetEngineStatus() == EngineStatus::Destroyed)
        {
            Logger::LogAppend("Game engine destruction detected, resetting lifecycle...");

            ThreadUtils::WaitOrExit(1000ms);
            if (!g_pState->Lifecycle.IsRunning()) break;

            if (g_pState->Lifecycle.ShouldAutoUpdatePhase() && g_pState->Lifecycle.GetCurrentPhase() != AutoTheaterPhase::Default)
            {
                AutoTheaterPhase targetPhase = g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Timeline ? AutoTheaterPhase::Director : AutoTheaterPhase::Timeline;
                if (g_pState->Theater.IsTheaterMode()) MainThread::UpdateToPhase(targetPhase);
            }

            EngineInitialize_Hook::Uninstall();
            DestroySubsystems_Hook::Uninstall();
            GameEngineStart_Hook::Uninstall();

            ThreadUtils::WaitOrExit(1000ms);
            if (!g_pState->Lifecycle.IsRunning())
            {
                Logger::LogAppend("MainThread: Shutdown in progress, skipping re-installation.");
                break;
            }

            if (!TryInstallLifecycleHooks("Engine Reset Cycle")) {
                Logger::LogAppend("ERROR: Failed to re-install hooks after engine reset!");
                ShutdownAndEject();
                return;
            }

            g_pState->Lifecycle.SetEngineStatus({ EngineStatus::Awaiting });
            g_pState->Theater.SetTheaterMode(false);
        }

        ThreadUtils::WaitOrExit(1000ms);
    }

    Logger::LogAppend("=== Main Thread Stopped ===");

    ResizeBuffers_Hook::Uninstall();
    Present_Hook::Uninstall();

    EngineInitialize_Hook::Uninstall();
    DestroySubsystems_Hook::Uninstall();
    GameEngineStart_Hook::Uninstall();

    std::this_thread::sleep_for(200ms);

    Logger::LogAppend("=== Cleanup complete. Ejecting mod... ===");

    HMODULE hMod = g_pState->Lifecycle.GetHandleModule();

    if (hMod != nullptr)
    {
        FreeLibraryAndExitThread(hMod, 0);
    }
}