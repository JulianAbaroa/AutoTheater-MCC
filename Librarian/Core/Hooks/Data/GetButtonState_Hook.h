#pragma once

typedef char(__fastcall* GetButtonState_t)(short buttonID);

namespace GetButtonState_Hook
{
	void Install();
	void Uninstall();
}