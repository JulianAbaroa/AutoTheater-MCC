#include "pch.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/Hooks/Memory/PlayersTableHook.h"
#include "Core/Hooks/Memory/TargetFramerateHook.h"
#include "Core/Hooks/Memory/ObjectsTableHook.h"
#include "Core/Hooks/Memory/EngineTimeHook.h"

CoreMemoryHook::CoreMemoryHook()
{
	PlayersTable = std::make_unique<PlayersTableHook>();
	TargetFramerate = std::make_unique<TargetFramerateHook>();
	ObjectsTable = std::make_unique<ObjectsTableHook>();
	EngineTime = std::make_unique<EngineTimeHook>();
}

CoreMemoryHook::~CoreMemoryHook() = default;