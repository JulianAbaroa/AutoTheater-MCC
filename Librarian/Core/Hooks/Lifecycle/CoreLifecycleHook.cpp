#include "pch.h"
#include "Core/Hooks/Lifecycle/CoreLifecycleHook.h"
#include "Core/Hooks/Lifecycle/DestroySubsystemsHook.h"
#include "Core/Hooks/Lifecycle/EngineInitializeHook.h"
#include "Core/Hooks/Lifecycle/GameEngineStartHook.h"

CoreLifecycleHook::CoreLifecycleHook()
{
	DestroySubsystems = std::make_unique<DestroySubsystemsHook>();
	EngineInitialize = std::make_unique<EngineInitializeHook>();
	GameEngineStart = std::make_unique<GameEngineStartHook>();
}

CoreLifecycleHook::~CoreLifecycleHook() = default;