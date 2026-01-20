#pragma once

#include <atomic>

extern std::atomic<bool> g_EngineHooksReady;

typedef void(__fastcall* EngineInitialize_t)(void);

namespace EngineInitialize_Hook
{
	bool Install();
	void Uninstall();
}