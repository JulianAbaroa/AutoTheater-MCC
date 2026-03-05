#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/Infrastructure/UserPreferencesSystem.h"
#include <fstream>

void UserPreferencesSystem::SavePreferences()
{
	if (!g_pState->Settings.ShouldUseAppData()) return;

	std::ofstream file(this->GetPreferencesFilePath(), std::ios::trunc);
	if (!file.is_open())
	{
		g_pUtil->Log.Append("[UserPreferencesSystem] ERROR: Failed to save user preferences.");
		return;
	}

	file << "; AutoTheater User Preferences\n";

	// FFmpegState
	file << "FFmpeg_TargetFramerate=" << g_pState->FFmpeg.GetTargetFramerate() << "\n";
	file << "FFmpeg_ShouldRecordUI=" << (g_pState->FFmpeg.ShouldRecordUI() ? "1" : "0") << "\n";
	file << "FFmpeg_OutputPath=" << g_pState->FFmpeg.GetOutputPath() << "\n";

	// LifecycleState
	file << "Lifecycle_AutoUpdatePhase=" << (g_pState->Lifecycle.ShouldAutoUpdatePhase() ? "1" : "0") << "\n";
	
	// SettingsState
	file << "Settings_ShouldFreezeMouse=" << (g_pState->Settings.ShouldFreezeMouse() ? "1" : "0") << "\n";
	file << std::fixed << std::setprecision(2);
	file << "Settings_MenuAlpha=" << g_pState->Settings.GetMenuAlpha() << "\n";
	file << "Settings_UIScale=" << g_pState->Render.GetUIScale() << "\n";

	file << std::defaultfloat;

	// UI
	file << "UI_TimelineAutoScroll=" << (g_pState->Settings.GetTimelineAutoScroll() ? "1" : "0") << "\n";
	file << "UI_TheaterAutoScroll=" << (g_pState->Settings.GetTheaterAutoScroll() ? "1" : "0") << "\n";
	file << "UI_DirectorAutoScroll=" << (g_pState->Settings.GetDirectorAutoScroll() ? "1" : "0") << "\n";
	file << "UI_LogsAutoScroll=" << (g_pState->Settings.GetLogsAutoScroll() ? "1" : "0") << "\n";

	g_pUtil->Log.Append("[UserPreferencesSystem] INFO: User preferences saved successfully.");
}

void UserPreferencesSystem::LoadPreferences()
{
	if (!g_pState->Settings.ShouldUseAppData()) return;

	std::ifstream file(this->GetPreferencesFilePath());
	if (!file.is_open())
	{
		g_pUtil->Log.Append("[UserPreferencesSystem] WARNING: No user preferences file found, using defaults.");
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		this->ParseLine(line);
	}

	g_pUtil->Log.Append("[UserPreferencesSystem] INFO: User preferences loaded successfully.");
}

void UserPreferencesSystem::ParseLine(const std::string& line)
{
	if (line.empty() || line[0] == '#' || line[0] == ';') return;

	auto delimiterPos = line.find('=');
	if (delimiterPos == std::string::npos) return;

	std::string key = line.substr(0, delimiterPos);
	std::string value = line.substr(delimiterPos + 1);

	// FFmpegState
	if (key == "FFmpeg_TargetFramerate") g_pState->FFmpeg.SetTargetFramerate(std::stof(value));
	else if (key == "FFmpeg_ShouldRecordUI") g_pState->FFmpeg.SetRecordUI(value == "1" || value == "true");
	else if (key == "FFmpeg_OutputPath") g_pState->FFmpeg.SetOutputPath(value);

	// LifecycleState
	else if (key == "Lifecycle_AutoUpdatePhase") g_pState->Lifecycle.SetAutoUpdatePhase(value == "1" || value == "true");

	// SettingsState
	else if (key == "Settings_ShouldFreezeMouse") g_pState->Settings.SetFreezeMouse(value == "1" || value == "true");
	else if (key == "Settings_MenuAlpha")
	{
		try 
		{
			g_pState->Settings.SetMenuAlpha(std::stof(value));
		}
		catch (...)
		{
			g_pState->Settings.SetMenuAlpha(1.0f);
		}
	}
	else if (key == "Settings_UIScale")
	{
		try
		{
			g_pState->Render.SetUIScale(std::stof(value));
		}
		catch (...)
		{
			g_pState->Render.SetUIScale(1.0f);
		}
	}

	// Tabs
	else if (key == "UI_TimelineAutoScroll") g_pState->Settings.SetTimelineAutoScroll(value == "1" || value == "true");
	else if (key == "UI_TheaterAutoScroll") g_pState->Settings.SetTheaterAutoScroll(value == "1" || value == "true");
	else if (key == "UI_DirectorAutoScroll") g_pState->Settings.SetDirectorAutoScroll(value == "1" || value == "true");
	else if (key == "UI_LogsAutoScroll") g_pState->Settings.SetLogsAutoScroll(value == "1" || value == "true");
}


std::string UserPreferencesSystem::GetPreferencesFilePath() const
{
	return g_pState->Settings.GetAppDataDirectory() + "\\user_preferences.cfg";
}