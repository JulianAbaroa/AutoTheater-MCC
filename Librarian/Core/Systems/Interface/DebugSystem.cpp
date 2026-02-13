#include "pch.h"
#include "DebugSystem.h"
#include "Core/Common/AppCore.h"

void DebugSystem::AddLog(std::string message)
{
	g_pState->Debug.PushBack(message);
	g_pState->Debug.TrimToSize(500);
}