#pragma once

#include <memory>

class BlamOpenFileHook;
class BuildGameEventHook;
class ReplayInitializeStateHook;
class SpectatorHandleInputHook;
class UpdateTelemetryTimerHook;

struct CoreDataHook
{
	CoreDataHook();
	~CoreDataHook();

	std::unique_ptr<BlamOpenFileHook> BlamOpenFile;
	std::unique_ptr<BuildGameEventHook> BuildGameEvent;
	std::unique_ptr<ReplayInitializeStateHook> ReplayInitializeState;
	std::unique_ptr<SpectatorHandleInputHook> SpectatorHandleInput;
	std::unique_ptr<UpdateTelemetryTimerHook> UpdateTelemetryTimer;
};