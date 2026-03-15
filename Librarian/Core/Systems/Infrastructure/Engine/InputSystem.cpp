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

    if (!g_pState->Infrastructure->Input->DequeueRequest(currentReq) ||
        currentReq.Action == InputAction::Unknown) 
    {
        return;
    }
    
    switch (currentReq.Action)
    {
    case InputAction::NextPlayer:
    case InputAction::PreviousPlayer:
    {
        uint8_t initialIdx = g_pState->Domain->Theater->GetSpectatedPlayerIndex();
    
        auto condition = [initialIdx]() {
            return g_pState->Domain->Theater->GetSpectatedPlayerIndex() != initialIdx;
        };  
    
        g_pSystem->Infrastructure->Input->InjectInput(currentReq, condition, 500ms, 20ms);
        return;
    }
    
    case InputAction::ToggleFreecam:
    {
        uint8_t initialCamState = g_pState->Domain->Theater->GetCameraMode();
    
        auto condition = [initialCamState]() {
            return g_pState->Domain->Theater->GetCameraMode() != initialCamState;
        };
    
        if (!g_pSystem->Infrastructure->Input->InjectInput(currentReq, condition, 500ms, 50ms)) 
        {
            g_pSystem->Debug->Log("[InputThread] WARNING: Freecam toggle timed out.");
        }

        return;
    }

    default:
        auto condition = []() { return false; };
        g_pSystem->Infrastructure->Input->InjectInput(currentReq, condition, 50ms, 50ms);
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

	std::this_thread::sleep_for(stabilizeMs);

	g_pState->Infrastructure->Input->SetProcessing(false);

	return success;
}