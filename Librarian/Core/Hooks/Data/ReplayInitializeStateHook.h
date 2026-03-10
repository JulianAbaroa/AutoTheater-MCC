#pragma once

#include <cstdint>
#include <atomic>

class ReplayInitializeStateHook
{
public:
	void Install();
	void Uninstall();

private:
	static void HookedReplayInitializeState(uint64_t sessionContext, uint64_t headerBuffer);

	typedef void(__fastcall* ReplayInitializeState_t)(uint64_t sessionContext, uint64_t headerBuffer);
	static inline ReplayInitializeState_t m_OriginalFunction = nullptr;

	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};