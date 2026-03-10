#pragma once

#include <memory>

class GetButtonStateHook;
class GetRawInputDataHook;

struct CoreInputHook
{
	CoreInputHook();
	~CoreInputHook();

	std::unique_ptr<GetButtonStateHook> GetButtonState;
	std::unique_ptr<GetRawInputDataHook> GetRawInputData;
};