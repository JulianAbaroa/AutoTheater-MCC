#pragma once

#include "Core/Common/Types/InputTypes.h"
#include <functional>
#include <map>

class InputSystem
{
public:
    void ManualInput();
    void AutomaticInput();
    
private:
    const std::map<int, float> INPUT_SPEED_MAP = {
        {'0', 16.0f},  {'9', 8.0f},   {'8', 4.0f},
        {'7', 1.0f},   {'6', 0.5f},   {'5', 0.25f},
        {'4', 0.1f},   {'3', 0.0f},
    };

	bool InjectInput(
        InputRequest req, 
        std::function<bool()> successCondition, 
        std::chrono::milliseconds timeoutMs, 
        std::chrono::milliseconds stabilizeMs
    );
};