#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Threads/MainThread.h"
#include <chrono>

using namespace std::chrono_literals;

void MainThread::Run() 
{
    // Initial delay.
    g_pUtil->Thread.WaitOrExit(5000ms);

    g_pUtil->Log.Append("[MainThread] INFO: Started.");

    this->InitializeAutoTheater();
    this->InstallCaptureHooks();

    while (g_pState->Lifecycle.IsRunning())
    {
        this->CheckHooksHealth();

        if (g_pState->Lifecycle.GetEngineStatus() == EngineStatus::Destroyed)
        {
            g_pUtil->Log.Append("[MainThread] INFO: Game engine destruction detected, resetting lifecycle.");

            if (!this->IsStillRunning()) break;

            if (g_pState->Lifecycle.ShouldAutoUpdatePhase() && g_pState->Lifecycle.GetCurrentPhase() != AutoTheaterPhase::Default)
            {
                AutoTheaterPhase targetPhase = g_pState->Lifecycle.GetCurrentPhase() == AutoTheaterPhase::Timeline ? AutoTheaterPhase::Director : AutoTheaterPhase::Timeline;
                if (g_pState->Theater.IsTheaterMode()) MainThread::UpdateToPhase(targetPhase);
            }

            this->UninstallLifecycleHooks();

            if (!this->IsStillRunning()) break;

            if (!this->TryInstallLifecycleHooks("Engine Reset Cycle")) 
            {
                g_pUtil->Log.Append("[MainThread] ERROR: Failed to re-install hooks after engine reset.");
                Shutdown();
                return;
            }

            g_pState->Lifecycle.SetEngineStatus({ EngineStatus::Waiting });
            g_pState->Theater.SetTheaterMode(false);
        }

        g_pUtil->Thread.WaitOrExit(1000ms);
    }

    this->UninstallCaptureHooks();
    this->UninstallLifecycleHooks();

    std::this_thread::sleep_for(200ms);
    g_pUtil->Log.Append("[MainThread] INFO: Stopped.");

    HMODULE hMod = g_pState->Lifecycle.GetHandleModule();
    if (hMod != nullptr) FreeLibraryAndExitThread(hMod, 0);
}

void MainThread::UpdateToPhase(AutoTheaterPhase targetPhase)
{
    if (targetPhase == g_pState->Lifecycle.GetCurrentPhase()) return;

    // Reset timeline
    if (targetPhase == AutoTheaterPhase::Timeline)
    {
        g_pSystem->Timeline.SetLastEventReached(false);

        g_pState->Timeline.ClearTimeline();
    }
    else if (targetPhase == AutoTheaterPhase::Director)
    {
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


void MainThread::InitializeAutoTheater()
{
    if (!this->TryInstallLifecycleHooks("Initial Boot"))
    {
        this->Shutdown();
        return;
    }

    g_pState->Lifecycle.SetCurrentPhase(g_pState->Settings.GetPreferredPhase());
    g_pSystem->EventRegistry.LoadEventRegistry();
}

void MainThread::UninstallLifecycleHooks()
{
    g_pHook->EngineInitialize.Uninstall();
    g_pHook->DestroySubsystems.Uninstall();
    g_pHook->GameEngineStart.Uninstall();
}

void MainThread::InstallCaptureHooks()
{
    // Video
    g_pHook->Present.Install();
    g_pHook->ResizeBuffers.Install();

    // Audio
    g_pHook->ReleaseBuffer.Install();
    g_pHook->GetBuffer.Install();
    g_pHook->AudioClientInitialize.Install();
    g_pHook->GetService.Install();
}

void MainThread::UninstallCaptureHooks()
{
    // Audio
    g_pHook->GetService.Uninstall();
    g_pHook->AudioClientInitialize.Uninstall();
    g_pHook->GetBuffer.Uninstall();
    g_pHook->ReleaseBuffer.Uninstall();

    // Video
    g_pHook->ResizeBuffers.Uninstall();
    g_pHook->Present.Uninstall();
}


void MainThread::CheckHooksHealth()
{
    bool areHooksIntact =
        !this->IsHookIntact(g_pHook->EngineInitialize.GetFunctionAddress()) ||
        !this->IsHookIntact(g_pHook->DestroySubsystems.GetFunctionAddress()) ||
        !this->IsHookIntact(g_pHook->GameEngineStart.GetFunctionAddress());

    if (areHooksIntact && g_pState->Lifecycle.IsRunning())
    {
        g_pUtil->Log.Append("[MainThread] WARNING: Hooks corrupted, rebooting.");
        g_pState->Lifecycle.SetEngineStatus({ EngineStatus::Destroyed });
    }
}

bool MainThread::IsStillRunning()
{
    g_pUtil->Thread.WaitOrExit(1000ms);
    if (!g_pState->Lifecycle.IsRunning()) return false;
    return true;
}


bool MainThread::IsHookIntact(void* address)
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

bool MainThread::TryInstallLifecycleHooks(const char* context)
{
    while (g_pState->Lifecycle.IsRunning())
    {
        if (g_pHook->EngineInitialize.Install(true) &&
            g_pHook->DestroySubsystems.Install(true) &&
            g_pHook->GameEngineStart.Install(true)) 
        {
            return true;
        }

        g_pUtil->Thread.WaitOrExit(1000ms);
    }

    return false;
}

void MainThread::Shutdown()
{
    g_pUtil->Log.Append("[MainThread] ERROR: Initiating emergency shutdown.");
    g_pState->Lifecycle.SetRunning(false);
}