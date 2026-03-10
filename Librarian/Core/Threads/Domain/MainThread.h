#pragma once

#include "Core/Common/Types/AppTypes.h"

class MainThread
{
public:
	void Run();

	void UpdateToPhase(Phase targetPhase);
	
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