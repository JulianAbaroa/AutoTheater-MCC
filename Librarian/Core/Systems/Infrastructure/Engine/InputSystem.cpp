#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/InputState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Theater/TheaterSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/InputSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <chrono>

using namespace std::chrono_literals;

void InputSystem::ManualInput()
{
	for (const auto& [key, speed] : g_pSystem->Infrastructure->Input->INPUT_SPEED_MAP)
	{
		if (GetAsyncKeyState(key) & 0x8000)
		{
			g_pSystem->Domain->Theater->SetReplaySpeed(speed);
			break;
		}
	}
}

void InputSystem::AutomaticInput()
{
    InputRequest currentReq = { InputContext::Unknown, InputAction::Unknown };

    auto replayModule = g_pState->Domain->Theater->GetModuleSnapshot();

    if (!g_pState->Infrastructure->Input->DequeueRequest(currentReq) ||
        currentReq.Action == InputAction::Unknown) return;
    
    switch (currentReq.Action)
    {
    case InputAction::NextPlayer:
    case InputAction::PreviousPlayer:
    {
        uint8_t initialIdx = (uint8_t)replayModule.FollowedPlayerIndex;
    
        auto condition = [initialIdx]() {
            auto currentModule = g_pState->Domain->Theater->GetModuleSnapshot();
            return std::to_integer<uint8_t>(currentModule.FollowedPlayerIndex) != initialIdx;
            };
    
        this->InjectInput(currentReq, condition, 200ms, 0ms);
        return;
    }
    
    case InputAction::ToggleCameraMode:
    {
        uint8_t initialCamState = (uint8_t)replayModule.CameraMode;
    
        auto condition = [initialCamState]() {
            auto currentModule = g_pState->Domain->Theater->GetModuleSnapshot();
            return (uint8_t)currentModule.CameraMode != initialCamState;
        };
    
        this->InjectInput(currentReq, condition, 200ms, 50ms);
        return;
    }

    case InputAction::ToggleUIMode:
    {
        uint8_t initialInterfaceState = (uint8_t)replayModule.UIMode;

        auto condition = [initialInterfaceState]() {
            auto currentModule = g_pState->Domain->Theater->GetModuleSnapshot();
            return (uint8_t)currentModule.UIMode != initialInterfaceState;
        };

        this->InjectInput(currentReq, condition, 200ms, 50ms);
        return;
    }

    case InputAction::TogglePOVMode:
    {
        uint8_t initialPOV = (uint8_t)replayModule.POVMode;

        auto condition = [initialPOV]() {
            auto currentModule = g_pState->Domain->Theater->GetModuleSnapshot();
            return (uint8_t)currentModule.POVMode != initialPOV;
        };

        this->InjectInput(currentReq, condition, 200ms, 50ms);
        return;
    }

    default:
        auto condition = []() { return false; };
        this->InjectInput(currentReq, condition, 100ms, 50ms);
        break;
    }
}

bool InputSystem::InjectInput(
    InputRequest request, 
    std::function<bool()> successCondition, 
    std::chrono::milliseconds timeoutMs,
    std::chrono::milliseconds stabilizeMs) 
{
	g_pState->Infrastructure->Input->SetNextRequest(request.Context, request.Action);

	auto startWait = std::chrono::steady_clock::now();
	bool success = false;

	while (std::chrono::steady_clock::now() - startWait < timeoutMs)
	{
		if (successCondition())
		{
			success = true;
			break;
		}

		std::this_thread::yield();
	}

	g_pState->Infrastructure->Input->SetNextRequest(InputContext::Theater, InputAction::Unknown);

    if (stabilizeMs > 0ms) std::this_thread::sleep_for(stabilizeMs);

	g_pState->Infrastructure->Input->SetProcessing(false);

	return success;
}