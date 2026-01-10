#pragma once

typedef void(__fastcall* SpectatorHandleInput_t)(
	uint64_t* pReplayModule,
	uint32_t frameDelta
);

namespace SpectatorHandleInput_Hook
{
	void Install();
	void Uninstall();
}