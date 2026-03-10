#include "pch.h"
#include "Core/Hooks/Data/CoreDataHook.h"
#include "Core/Hooks/Data/UpdateTelemetryTimerHook.h"
#include "Core/Hooks/Data/SpectatorHandleInputHook.h"
#include "Core/Hooks/Data/ReplayInitializeStateHook.h"
#include "Core/Hooks/Data/BuildGameEventHook.h"
#include "Core/Hooks/Data/BlamOpenFileHook.h"

CoreDataHook::CoreDataHook()
{
    BlamOpenFile = std::make_unique<BlamOpenFileHook>();
    BuildGameEvent = std::make_unique<BuildGameEventHook>();
    ReplayInitializeState = std::make_unique<ReplayInitializeStateHook>();
    SpectatorHandleInput = std::make_unique<SpectatorHandleInputHook>();
    UpdateTelemetryTimer = std::make_unique<UpdateTelemetryTimerHook>();
}

CoreDataHook::~CoreDataHook() = default;