#include "pch.h"
#include "Utils/ThreadUtils.h"

bool ThreadUtils::WaitOrExit(std::chrono::milliseconds ms)
{
	std::unique_lock lock(g_pState->shutdownMutex);

	return g_pState->shutdownCV.wait_for(lock, ms, [] {
		return !g_pState->running.load();
	});
}