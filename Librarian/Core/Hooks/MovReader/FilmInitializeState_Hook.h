#pragma once

#include <cstdint>

typedef void(__fastcall* FilmInitializeState_t)(
	uint64_t sessionContext,
	uint64_t headerBuffer
);

namespace FilmInitializeState_Hook
{
	void Install();
	void Uninstall();
}