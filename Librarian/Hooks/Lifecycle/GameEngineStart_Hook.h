#pragma once

extern bool g_IsTheaterMode;
extern void* g_GameEngineStart_Address;

typedef void(__fastcall* GameEngineStart_t)(uint64_t, uint64_t, uint64_t*);

namespace GameEngineStart_Hook
{
	bool Install(bool silent);
	void Uninstall();
}