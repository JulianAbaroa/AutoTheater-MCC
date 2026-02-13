#pragma once

extern void* g_DestroySubsystems_Address;

typedef void(__fastcall* DestroySubsystems_t)(void);

namespace DestroySubsystems_Hook
{
	bool Install(bool silent);
	void Uninstall();
}