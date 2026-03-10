#include "pch.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/Hooks/Memory/PlayersTableHook.h"
#include "Core/Hooks/Memory/TargetFramerateHook.h"
#include "Core/Hooks/Memory/ObjectsTableHook.h"

CoreMemoryHook::CoreMemoryHook()
{
	PlayersTable = std::make_unique<PlayersTableHook>();
	TargetFramerate = std::make_unique<TargetFramerateHook>();
	ObjectsTable = std::make_unique<ObjectsTableHook>();
}

CoreMemoryHook::~CoreMemoryHook() = default;