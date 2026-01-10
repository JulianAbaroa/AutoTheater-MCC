#pragma once

typedef void(__fastcall* EngineInitialize_t)(void);

namespace EngineInitialize_Hook
{
	bool Install();
	void Uninstall();
}