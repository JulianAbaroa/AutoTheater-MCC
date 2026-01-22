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
    g_Running.store(false);

    std::thread([=]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        FreeLibraryAndExitThread(g_HandleModule, 0);
    }).detach();
}

static bool TryInstallLifecycleHooks(const char* context)
{
    std::stringstream ss;

    while (g_Running.load())
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

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return false;
}

static void UpdatePhase()
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
        if (!IsHookIntact(g_EngineInitialize_Address) ||
            !IsHookIntact(g_DestroySubsystems_Address) ||
            !IsHookIntact(g_GameEngineStart_Address)
        ) {
            Logger::LogAppend("Watchdog: Hook corrupted or memory restored. Rebooting...");
            g_GameEngineDestroyed = true;
        }

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