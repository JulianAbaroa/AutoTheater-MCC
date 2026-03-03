#pragma once

#include "Core/Common/Types/AutoTheaterTypes.h"
#include <thread>

class MainThread
{
public:
	void Run();

	void UpdateToPhase(AutoTheaterPhase targetPhase);

private:
	void InitializeAutoTheater();
	void UninstallLifecycleHooks();

	void InstallCaptureHooks();
	void UninstallCaptureHooks();

	void CheckHooksHealth();
	bool IsStillRunning();

	bool TryInstallLifecycleHooks(const char* context);
	bool IsHookIntact(void* address);
	void Shutdown();
};