#pragma once

#include <memory>

class MainInterface;
class LogsTab;
class CaptureTab;
class EventRegistryTab;
class ReplayManagerTab;
class DirectorTab;
class SettingsTab;
class TheaterTab;
class TimelineTab;

struct CoreUI
{
	CoreUI();
	~CoreUI();

	std::unique_ptr<MainInterface> Main;
	std::unique_ptr<LogsTab> Logs;
	std::unique_ptr<CaptureTab> Capture;
	std::unique_ptr<EventRegistryTab> EventRegistry;
	std::unique_ptr<ReplayManagerTab> Replay;
	std::unique_ptr<DirectorTab> Director;
	std::unique_ptr<SettingsTab> Settings;
	std::unique_ptr<TheaterTab> Theater;
	std::unique_ptr<TimelineTab> Timeline;
};

extern CoreUI* g_pUI;