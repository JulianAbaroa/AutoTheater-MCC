#include "pch.h"
#include "Core/Hooks/Input/CoreInputHook.h"
#include "Core/Hooks/Input/GetButtonStateHook.h"
#include "Core/Hooks/Input/GetRawInputDataHook.h"

CoreInputHook::CoreInputHook()
{
	GetButtonState = std::make_unique<GetButtonStateHook>();
	GetRawInputData = std::make_unique<GetRawInputDataHook>();
}

CoreInputHook::~CoreInputHook() = default;