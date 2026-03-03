#pragma once

#include <cstdint>
#include <atomic>

class FilmInitializeStateHook
{
public:
	void Install();
	void Uninstall();

private:
	static void HookedFilmInitializeState(uint64_t sessionContext, uint64_t headerBuffer);

	typedef void(__fastcall* FilmInitializeState_t)(uint64_t sessionContext, uint64_t headerBuffer);
	static inline FilmInitializeState_t m_OriginalFunction = nullptr;

	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};