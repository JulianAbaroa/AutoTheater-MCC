#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/AppCore.h"
#include "Core/Systems/Interface/InputSystem.h"
#include <chrono>

using namespace std::chrono_literals;

void InputSystem::ManualInput()
{
	for (const auto& [key, speed] : g_pSystem->Input.INPUT_SPEED_MAP)
	{
		if (GetAsyncKeyState(key) & 0x8000)
		{
			g_pSystem->Theater.SetReplaySpeed(speed);
			break;
		}
	}
}

void InputSystem::AutomaticInput()
{
    InputRequest currentReq = { InputContext::Unknown, InputAction::Unknown };

    if (!g_pState->Input.DequeueRequest(currentReq) ||
        currentReq.Action == InputAction::Unknown
        ) {
        return;
    }
    
    switch (currentReq.Action)
    {
    case InputAction::NextPlayer:
    case InputAction::PreviousPlayer:
    {
        uint8_t initialIdx = g_pState->Theater.GetSpectatedPlayerIndex();
    
        auto condition = [initialIdx]() {
            return g_pState->Theater.GetSpectatedPlayerIndex() != initialIdx;
        };  
    
        g_pSystem->Input.InjectInput(currentReq, condition, 500ms, 20ms);
        return;
    }
    
    case InputAction::ToggleFreecam:
    {
        uint8_t initialCamState = g_pState->Theater.GetCameraMode();
    
        auto condition = [initialCamState]() {
            return g_pState->Theater.GetCameraMode() != initialCamState;
        };
    
        if (!g_pSystem->Input.InjectInput(currentReq, condition, 500ms, 50ms)) {
            Logger::LogAppend("[InputThread] Warning: Freecam toggle timed out.");
        }

        return;
    }

    default:
        auto condition = []() { return false; };
        g_pSystem->Input.InjectInput(currentReq, condition, 50ms, 50ms);
        break;
    }
}

bool InputSystem::InjectInput(
    InputRequest request, 
    std::function<bool()> successCondition, 
    std::chrono::milliseconds timeoutMs,
    std::chrono::milliseconds stabilizeMs
) {
	g_pState->Input.SetNextRequest(request.Context, request.Action);

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

	g_pState->Input.SetNextRequest(InputContext::Theater, InputAction::Unknown);

	std::this_thread::sleep_for(stabilizeMs);

	g_pState->Input.SetProcessing(false);

	return success;
}