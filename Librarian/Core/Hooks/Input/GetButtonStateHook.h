#pragma once

#include <atomic>

class GetButtonStateHook
{
public:
	void Install();
	void Uninstall();

private:
	static char __fastcall HookedGetButtonState(short buttonID);

	typedef char(__fastcall* GetButtonState_t)(short buttonID);

	static inline GetButtonState_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};
