#pragma once

#include "Core/Hooks/Audio/AudioClientInitializeHook.h"
#include "Core/Hooks/Audio/ReleaseBufferHook.h"
#include "Core/Hooks/Audio/GetServiceHook.h"
#include "Core/Hooks/Audio/GetBufferHook.h"
#include "Core/Hooks/Data/UpdateTelemetryTimerHook.h"
#include "Core/Hooks/Data/SpectatorHandleInputHook.h"
#include "Core/Hooks/Data/FilmInitializeStateHook.h"
#include "Core/Hooks/Data/BuildGameEventHook.h"
#include "Core/Hooks/Data/BlamOpenFileHook.h"
#include "Core/Hooks/Lifecycle/DestroySubsystemsHook.h"
#include "Core/Hooks/Lifecycle/EngineInitializeHook.h"
#include "Core/Hooks/Lifecycle/GameEngineStartHook.h"
#include "Core/Hooks/Tables/TelemetryTables.h"
#include "Core/Hooks/Input/GetButtonStateHook.h"
#include "Core/Hooks/Input/GetRawInputDataHook.h"
#include "Core/Hooks/Render/ResizeBuffersHook.h"
#include "Core/Hooks/Render/PresentHook.h"
#include "Core/Hooks/Window/WndProcHook.h"
#include "Core/Hooks/Tables/TargetFPS.h"

struct CoreHook
{
	// Audio
	AudioClientInitializeHook AudioClientInitialize;
	ReleaseBufferHook ReleaseBuffer;
	GetServiceHook GetService;
	GetBufferHook GetBuffer;

	// Data
	UpdateTelemetryTimerHook UpdateTelemetryTimer;
	SpectatorHandleInputHook SpectatorHandleInput;
	FilmInitializeStateHook FilmInitializeState;
	BuildGameEventHook BuildGameEvent;
	BlamOpenFileHook BlamOpenFile;

	// Lifecycle
	DestroySubsystemsHook DestroySubsystems;
	EngineInitializeHook EngineInitialize;
	GameEngineStartHook GameEngineStart;

	// Memory
	TelemetryTables Tables;
	TargetFPS TargetFPS;

	// Input
	GetButtonStateHook GetButtonState;
	GetRawInputDataHook GetRawInputData;

	// Render
	ResizeBuffersHook ResizeBuffers;
	PresentHook Present;

	// Window
	WndProcHook WndProc;
};

extern CoreHook* g_pHook;