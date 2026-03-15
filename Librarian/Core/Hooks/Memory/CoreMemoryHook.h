#pragma once

#include <memory>

class PlayersTableHook;
class TargetFramerateHook;
class ObjectsTableHook;
class EngineTimeHook;

struct CoreMemoryHook
{
	CoreMemoryHook();
	~CoreMemoryHook();

	std::unique_ptr<PlayersTableHook> PlayersTable;
	std::unique_ptr<TargetFramerateHook> TargetFramerate;
	std::unique_ptr<ObjectsTableHook> ObjectsTable;
	std::unique_ptr<EngineTimeHook> EngineTime;
};