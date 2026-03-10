#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Lifecycle/CoreLifecycleHook.h"
#include "Core/Hooks/Lifecycle/EngineInitializeHook.h"
#include "Core/Hooks/Lifecycle/DestroySubsystemsHook.h"
#include "Core/Hooks/Lifecycle/GameEngineStartHook.h"
#include "Core/Hooks/Render/CoreRenderHook.h"
#include "Core/Hooks/Render/PresentHook.h"
#include "Core/Hooks/Render/ResizeBuffersHook.h"
#include "Core/Hooks/Audio/CoreAudioHook.h"
#include "Core/Hooks/Audio/AudioClientInitializeHook.h"
#include "Core/Hooks/Audio/ReleaseBufferHook.h"
#include "Core/Hooks/Audio/GetBufferHook.h"
#include "Core/Hooks/Audio/GetServiceHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Director/DirectorState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Domain/Timeline/TimelineState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/InputState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Director/DirectorSystem.h"
#include "Core/Systems/Domain/Director/EventRegistrySystem.h"
#include "Core/Systems/Domain/Timeline/TimelineSystem.h"
#include "Core/Threads/Domain/MainThread.h"
#include <chrono>

using namespace std::chrono_literals;

void MainThread::Run() 
{
    // Initial delay.
    g_pUtil->Thread.WaitOrExit(5000ms);

    g_pUtil->Log.Append("[MainThread] INFO: Started.");

    this->InitializeAutoTheater();
    this->InstallCaptureHooks();

    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        this->CheckHooksHealth();

        if (g_pState->Infrastructure->Lifecycle->GetEngineStatus() == EngineStatus::Destroyed)
        {
            g_pUtil->Log.Append("[MainThread] INFO: Game engine destruction detected, resetting lifecycle.");

            if (!this->IsStillRunning()) break;

            if (g_pState->Infrastructure->Lifecycle->ShouldAutoUpdatePhase() && 
                g_pState->Infrastructure->Lifecycle->GetCurrentPhase() != Phase::Default)
            {
                Phase targetPhase = g_pState->Infrastructure->Lifecycle->GetCurrentPhase() == Phase::Timeline ? Phase::Director : Phase::Timeline;
                if (g_pState->Domain->Theater->IsTheaterMode()) MainThread::UpdateToPhase(targetPhase);
            }

            this->UninstallLifecycleHooks();

            if (!this->IsStillRunning()) break;

            if (!this->TryInstallLifecycleHooks("Engine Reset Cycle")) 
            {
                g_pUtil->Log.Append("[MainThread] ERROR: Failed to re-install hooks after engine reset.");
                Shutdown();
                return;
            }

            g_pState->Infrastructure->Lifecycle->SetEngineStatus({ EngineStatus::Waiting });
            g_pState->Domain->Theater->SetTheaterMode(false);
        }

        g_pUtil->Thread.WaitOrExit(1000ms);
    }

    this->UninstallCaptureHooks();
    this->UninstallLifecycleHooks();

    std::this_thread::sleep_for(200ms);
    g_pUtil->Log.Append("[MainThread] INFO: Stopped.");

    HMODULE hMod = g_pState->Infrastructure->Lifecycle->GetHandleModule();
    if (hMod != nullptr) FreeLibraryAndExitThread(hMod, 0);
}

void MainThread::UpdateToPhase(Phase targetPhase)
{
    if (targetPhase == g_pState->Infrastructure->Lifecycle->GetCurrentPhase()) return;

    // Reset timeline
    if (targetPhase == Phase::Timeline)
    {
        g_pSystem->Domain->Timeline->SetLastEventReached(false);

        g_pState->Domain->Timeline->ClearTimeline();
    }
    else if (targetPhase == Phase::Director)
    {
        g_pSystem->Domain->Timeline->SetLastEventReached(true);
    }
    g_pSystem->Domain->Timeline->SetLoggedEventsCount(0);

    // Reset theater
    g_pState->Domain->Theater->ResetPlayerList();

    // Reset director
    g_pState->Domain->Director->SetInitialized(false);
    g_pState->Domain->Director->SetHooksReady(false);
    g_pSystem->Domain->Director->SetCurrentCommandIndex(0);
    g_pSystem->Domain->Director->SetLastReplayTime(0.0f);
    g_pState->Domain->Director->ClearScript();

    // Reset input
    g_pState->Infrastructure->Input->Reset();

    // Update phase
    g_pState->Infrastructure->Lifecycle->SetCurrentPhase(targetPhase);
}


void MainThread::InitializeAutoTheater()
{
    if (!this->TryInstallLifecycleHooks("Initial Boot"))
    {
        this->Shutdown();
        return;
    }

    g_pState->Infrastructure->Lifecycle->SetCurrentPhase(g_pState->Infrastructure->Settings->GetPreferredPhase());
    g_pSystem->Domain->EventRegistry->LoadEventRegistry();
}

void MainThread::UninstallLifecycleHooks()
{
    g_pHook->Lifecycle->EngineInitialize->Uninstall();
    g_pHook->Lifecycle->DestroySubsystems->Uninstall();
    g_pHook->Lifecycle->GameEngineStart->Uninstall();
}

void MainThread::InstallCaptureHooks()
{
    // Video
    g_pHook->Render->Present->Install();
    g_pHook->Render->ResizeBuffers->Install();

    // Audio
    g_pHook->Audio->ReleaseBuffer->Install();
    g_pHook->Audio->GetBuffer->Install();
    g_pHook->Audio->AudioClientInitialize->Install();
    g_pHook->Audio->GetService->Install();
}

void MainThread::UninstallCaptureHooks()
{
    // Audio
    g_pHook->Audio->GetService->Uninstall();
    g_pHook->Audio->AudioClientInitialize->Uninstall();
    g_pHook->Audio->GetBuffer->Uninstall();
    g_pHook->Audio->ReleaseBuffer->Uninstall();

    // Video
    g_pHook->Render->ResizeBuffers->Uninstall();
    g_pHook->Render->Present->Uninstall();
}


void MainThread::CheckHooksHealth()
{
    bool areHooksIntact =
        !this->IsHookIntact(g_pHook->Lifecycle->EngineInitialize->GetFunctionAddress()) ||
        !this->IsHookIntact(g_pHook->Lifecycle->DestroySubsystems->GetFunctionAddress()) ||
        !this->IsHookIntact(g_pHook->Lifecycle->GameEngineStart->GetFunctionAddress());

    if (areHooksIntact && g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        g_pUtil->Log.Append("[MainThread] WARNING: Hooks corrupted, rebooting.");
        g_pState->Infrastructure->Lifecycle->SetEngineStatus({ EngineStatus::Destroyed });
    }
}

bool MainThread::IsStillRunning()
{
    g_pUtil->Thread.WaitOrExit(1000ms);
    if (!g_pState->Infrastructure->Lifecycle->IsRunning()) return false;
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
    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        if (g_pHook->Lifecycle->EngineInitialize->Install(true) &&
            g_pHook->Lifecycle->DestroySubsystems->Install(true) &&
            g_pHook->Lifecycle->GameEngineStart->Install(true)) 
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
    g_pState->Infrastructure->Lifecycle->SetRunning(false);
}