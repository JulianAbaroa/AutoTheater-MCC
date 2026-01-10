#pragma once

extern bool g_GameEngineDestroyed;

typedef void(__fastcall* DestroySubsystems_t)(void);

namespace DestroySubsystems_Hook
{
	bool Install();
	void Uninstall();
}