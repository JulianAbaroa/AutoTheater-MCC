#pragma once

#include <atomic>

class DestroySubsystemsHook
{
public:
	bool Install(bool silent);
	void Uninstall();

	void* GetFunctionAddress();

private:
	static void __fastcall HookedDestroySubsystems(void);

	typedef void(__fastcall* DestroySubsystems_t)(void);

	static inline DestroySubsystems_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};