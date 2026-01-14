#pragma once

extern bool g_IsTheaterMode;

typedef void(__fastcall* GameEngineStart_t)(uint64_t, uint64_t, uint64_t*);

namespace GameEngineStart_Hook
{
	bool Install();
	void Uninstall();
}