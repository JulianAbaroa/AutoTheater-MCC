#pragma once

#include "Core/UI/MainInterface.h"
#include "Core/UI/Tabs/Logs/LogsTab.h"
#include "Core/UI/Tabs/Optional/CaptureTab.h"
#include "Core/UI/Tabs/Optional/EventRegistryTab.h"
#include "Core/UI/Tabs/Optional/ReplayManagerTab.h"
#include "Core/UI/Tabs/Primary/DirectorTab.h"
#include "Core/UI/Tabs/Primary/SettingsTab.h"
#include "Core/UI/Tabs/Primary/TheaterTab.h"
#include "Core/UI/Tabs/Primary/TimelineTab.h"

struct CoreUI
{
	MainInterface Main;

	// Logs
	LogsTab Logs;

	// Optional
	CaptureTab Capture;
	EventRegistryTab EventRegistry;
	ReplayManagerTab Replay;

	// Primary
	DirectorTab Director;
	SettingsTab Settings;
	TheaterTab Theater;
	TimelineTab Timeline;
};

extern CoreUI* g_pUI;