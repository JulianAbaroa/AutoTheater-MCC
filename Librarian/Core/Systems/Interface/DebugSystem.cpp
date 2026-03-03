#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"

void DebugSystem::AddLog(LogEntry entry)
{
	g_pState->Debug.PushBack(entry);
	g_pState->Debug.TrimToSize(g_pState->Debug.GetMaxCapacity());
}