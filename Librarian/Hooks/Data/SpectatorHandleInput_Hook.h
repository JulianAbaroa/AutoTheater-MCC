#pragma once

extern volatile uint8_t g_FollowedPlayerIdx;
extern uintptr_t g_pReplayModule;

typedef void(__fastcall* SpectatorHandleInput_t)(
	uint64_t* pReplayModule,
	uint32_t frameDelta
);

namespace SpectatorHandleInput_Hook
{
	void Install();
	void Uninstall();
}