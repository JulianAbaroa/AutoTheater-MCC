#pragma once

extern void* g_EngineInitialize_Address;

typedef void(__fastcall* EngineInitialize_t)(void);

namespace EngineInitialize_Hook
{
	bool Install(bool silent);
	void Uninstall();
}