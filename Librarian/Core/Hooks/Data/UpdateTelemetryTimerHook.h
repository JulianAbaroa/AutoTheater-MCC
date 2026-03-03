#pragma once

#include <cstdint>
#include <atomic>

class UpdateTelemetryTimerHook
{
public:
	void Install();
	void Uninstall();

private:
	static void HookedUpdateTelemetryTimer(uint64_t timerContext, float deltaTime);
	
	typedef void(__fastcall* UpdateTelemetryTimer_t)(
		uint64_t timerContext, float deltaTime);

	static inline UpdateTelemetryTimer_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};