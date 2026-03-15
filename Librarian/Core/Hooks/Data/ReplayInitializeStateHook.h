#pragma once

#include <cstdint>
#include <atomic>
#include <string>

class ReplayInitializeStateHook
{
public:
	void Install();
	void Uninstall();

private:
	static void HookedReplayInitializeState(uint64_t sessionContext, uint64_t headerBuffer);
	static std::string ToCompactAlpha(const std::wstring& ws);

	typedef void(__fastcall* ReplayInitializeState_t)(uint64_t sessionContext, uint64_t headerBuffer);
	static inline ReplayInitializeState_t m_OriginalFunction = nullptr;

	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};