#pragma once

#include <atomic>

class EngineInitializeHook
{
public:
	bool Install(bool silent);
	void Uninstall();

	void* GetFunctionAddress();

private:
	static void __fastcall HookedEngineInitialize(void);
	
	typedef void(__fastcall* EngineInitialize_t)(void);

	static inline EngineInitialize_t m_OriginalFunction = nullptr;
	std::atomic<bool> m_IsHookInstalled{ false };
	std::atomic<void*> m_FunctionAddress{ nullptr };
};