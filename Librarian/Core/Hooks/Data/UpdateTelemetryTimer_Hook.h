#pragma once

#include <cstdint>

typedef void(__fastcall* UpdateTelemetryTimer_t)(
	uint64_t timerContext,
	float deltaTime
);

namespace UpdateTelemetryTimer_Hook
{
	void Install();
	void Uninstall();
}