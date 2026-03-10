#pragma once

#include <memory>

class DestroySubsystemsHook;
class EngineInitializeHook;
class GameEngineStartHook;

struct CoreLifecycleHook
{
	CoreLifecycleHook();
	~CoreLifecycleHook();

	std::unique_ptr<DestroySubsystemsHook> DestroySubsystems;
	std::unique_ptr<EngineInitializeHook> EngineInitialize;
	std::unique_ptr<GameEngineStartHook> GameEngineStart;
};