#include "pch.h"
#include "Core/UI/CoreUI.h"
#include "Core/UI/MainInterface.h"
#include "Core/UI/Tabs/Logs/LogsTab.h"
#include "Core/UI/Tabs/Optional/CaptureTab.h"
#include "Core/UI/Tabs/Optional/EventRegistryTab.h"
#include "Core/UI/Tabs/Optional/ReplayManagerTab.h"
#include "Core/UI/Tabs/Primary/DirectorTab.h"
#include "Core/UI/Tabs/Primary/SettingsTab.h"
#include "Core/UI/Tabs/Primary/TheaterTab.h"
#include "Core/UI/Tabs/Primary/TimelineTab.h"

CoreUI::CoreUI()
{
	Main = std::make_unique<MainInterface>();
	Logs = std::make_unique<LogsTab>();
	Capture = std::make_unique<CaptureTab>();
	EventRegistry = std::make_unique<EventRegistryTab>();
	Replay = std::make_unique<ReplayManagerTab>();
	Director = std::make_unique<DirectorTab>();
	Settings = std::make_unique<SettingsTab>();
	Theater = std::make_unique<TheaterTab>();
	Timeline = std::make_unique<TimelineTab>();
}

CoreUI::~CoreUI() = default;