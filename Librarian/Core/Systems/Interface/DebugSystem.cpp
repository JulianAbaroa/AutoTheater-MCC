#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Interface/DebugState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

void DebugSystem::AddLog(LogEntry entry)
{
	g_pState->Debug->PushBack(entry);
	g_pState->Debug->TrimToSize(g_pState->Debug->GetMaxCapacity());
}

void DebugSystem::RemoveLogsIf(std::function<bool(const LogEntry&)> predicate)
{
	g_pState->Debug->RemoveIf(predicate);
}