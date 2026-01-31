#include "pch.h"
#include "Utils/ThreadUtils.h"

bool ThreadUtils::WaitOrExit(std::chrono::milliseconds ms)
{
	std::unique_lock lock(g_pState->ShutdownMutex);

	return g_pState->ShutdownCV.wait_for(lock, ms, [] {
		return !g_pState->Running.load();
	});
}