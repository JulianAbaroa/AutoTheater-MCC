#pragma once

#include "Core/Common/Types/BlamTypes.h"
#include <cstdint>
#include <atomic>

class SpectatorHandleInputHook
{
public:
	void Install();
	void Uninstall();

private:
	static void __fastcall HookedSpectatorHandleInput(ReplayModule* pReplayModule, uint32_t rdx_param);

	typedef void(__fastcall* SpectatorHandleInput_t)(ReplayModule* pReplayModule, uint32_t frameDelta);

	static inline SpectatorHandleInput_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};