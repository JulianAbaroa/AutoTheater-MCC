#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Theater/TheaterState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/Persistence/PreferencesSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <fstream>

void PreferencesSystem::SavePreferences()
{
	if (!g_pState->Infrastructure->Settings->ShouldUseAppData()) return;

	std::ofstream file(this->GetPreferencesFilePath(), std::ios::trunc);
	if (!file.is_open())
	{
		g_pSystem->Debug->Log("[PreferencesSystem] ERROR: Failed to save user preferences.");
		return;
	}

	file << "; AutoTheater User Preferences\n";

	this->SaveFFmpegState(file);
	this->SaveLifeCycleState(file);
	this->SaveSettingsState(file);
	this->SaveUI(file);

	g_pSystem->Debug->Log("[PreferencesSystem] INFO: User preferences saved successfully.");
}

void PreferencesSystem::LoadPreferences()
{
	if (!g_pState->Infrastructure->Settings->ShouldUseAppData()) return;

	std::ifstream file(this->GetPreferencesFilePath());
	if (!file.is_open())
	{
		g_pSystem->Debug->Log("[PreferencesSystem] WARNING: No user preferences file found, using defaults.");
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		this->ParseLine(line);
	}

	g_pSystem->Debug->Log("[PreferencesSystem] INFO: User preferences loaded successfully.");
}


void PreferencesSystem::ParseLine(const std::string& line)
{
	if (line.empty() || line[0] == '#' || line[0] == ';') return;

	auto delimiterPos = line.find('=');
	if (delimiterPos == std::string::npos) return;

	std::string key = line.substr(0, delimiterPos);
	std::string value = line.substr(delimiterPos + 1);

	if (key.find("FFmpeg_") == 0) this->LoadFFmpegState(key, value);
	else if (key.find("Lifecycle_") == 0) this->LoadLifecycleState(key, value);
	else if (key.find("Settings_") == 0) this->LoadSettingsState(key, value);
	else if (key.find("UI_") == 0) this->LoadUI(key, value);
}

std::string PreferencesSystem::GetPreferencesFilePath() const
{
	return g_pState->Infrastructure->Settings->GetAppDataDirectory() + "\\user_preferences.cfg";
}


void PreferencesSystem::SaveFFmpegState(std::ofstream& file)
{
	auto encoderOptions = g_pState->Infrastructure->FFmpeg->GetEncoderConfig();

	file << "FFmpeg_ResolutionType=" << static_cast<int>(g_pState->Infrastructure->FFmpeg->GetResolutionType()) << "\n";

	file << std::fixed << std::setprecision(2);
	file << "FFmpeg_TargetFramerate=" << g_pState->Infrastructure->FFmpeg->GetTargetFramerate() << "\n";
	file << std::defaultfloat;

	file << "FFmpeg_ShouldRecordUI=" << (g_pState->Infrastructure->FFmpeg->ShouldRecordUI() ? "1" : "0") << "\n";
	file << "FFmpeg_OutputPath=" << g_pState->Infrastructure->FFmpeg->GetOutputPath() << "\n";
	file << "FFmpeg_StopOnLastEvent=" << g_pState->Infrastructure->FFmpeg->StopOnLastEvent() << "\n";

	file << std::fixed << std::setprecision(2);
	file << "FFmpeg_StopDelayDuration=" << g_pState->Infrastructure->FFmpeg->GetStopDelayDuration() << "\n";
	file << std::defaultfloat;

	file << "FFmpeg_ThreadQueueSize=" << encoderOptions.ThreadQueueSize << "\n";
	file << "FFmpeg_BitrateKbps=" << encoderOptions.BitrateKbps << "\n";
	file << "FFmpeg_VideoBufferPipeSize=" << encoderOptions.VideoBufferPipeSize << "\n";
	file << "FFmpeg_VideoPreset=" << static_cast<int>(encoderOptions.VideoPreset) << "\n";
	file << "FFmpeg_ScalingFilter=" << static_cast<int>(encoderOptions.ScalingFilter) << "\n";
	file << "FFmpeg_OutputContainer=" << static_cast<int>(encoderOptions.OutputContainer) << "\n";
	file << "FFmpeg_EncoderType=" << static_cast<int>(encoderOptions.EncoderType) << "\n";
}

void PreferencesSystem::SaveLifeCycleState(std::ofstream& file)
{
	file << "Lifecycle_AutoUpdatePhase=" << (g_pState->Infrastructure->Lifecycle->ShouldAutoUpdatePhase() ? "1" : "0") << "\n";
}

void PreferencesSystem::SaveSettingsState(std::ofstream& file)
{
	file << "Settings_ShouldFreezeMouse=" << (g_pState->Infrastructure->Settings->ShouldFreezeMouse() ? "1" : "0") << "\n";
	file << "Settings_ReplaySpeedModifierKeys=" << (g_pState->Infrastructure->Settings->ShouldUseManualInput() ? "1" : "0") << "\n";
	
	file << std::fixed << std::setprecision(2);
	file << "Settings_MenuAlpha=" << g_pState->Infrastructure->Settings->GetMenuAlpha() << "\n";
	file << "Settings_UIScale=" << g_pState->Infrastructure->Render->GetUIScale() << "\n";
	file << std::defaultfloat;

	file << "Settings_PreferredPhase=" << static_cast<int>(g_pState->Infrastructure->Settings->GetPreferredPhase()) << "\n";
}

void PreferencesSystem::SaveUI(std::ofstream& file)
{
	file << "UI_TimelineAutoScroll=" << (g_pState->Infrastructure->Settings->GetTimelineAutoScroll() ? "1" : "0") << "\n";
	file << "UI_TheaterAutoScroll=" << (g_pState->Infrastructure->Settings->GetTheaterAutoScroll() ? "1" : "0") << "\n";
	file << "UI_DirectorAutoScroll=" << (g_pState->Infrastructure->Settings->GetDirectorAutoScroll() ? "1" : "0") << "\n";
	file << "UI_LogsAutoScroll=" << (g_pState->Infrastructure->Settings->GetLogsAutoScroll() ? "1" : "0") << "\n";
	file << "UI_LockThirdPersonPOV=" << (g_pState->Domain->Theater->IsThirdPersonForced() ? "1" : "0") << "\n";
}

void PreferencesSystem::LoadFFmpegState(std::string& key, std::string& value)
{
	FFmpegEncoderConfig encoderConfig = g_pState->Infrastructure->FFmpeg->GetEncoderConfig();

	if (key == "FFmpeg_ResolutionType")
	{
		try
		{
			int resolutionInt = std::stoi(value);
			if (resolutionInt >= 0 && resolutionInt <= 2)
			{
				g_pState->Infrastructure->FFmpeg->SetResolutionType(static_cast<ResolutionType>(resolutionInt));
			}
		}
		catch (...)
		{
			g_pState->Infrastructure->FFmpeg->SetResolutionType(ResolutionType::FullHD);
		}
	}
	else if (key == "FFmpeg_TargetFramerate")
	{
		try
		{
			g_pState->Infrastructure->FFmpeg->SetTargetFramerate(std::stof(value));
		}
		catch (...)
		{
			g_pState->Infrastructure->FFmpeg->SetTargetFramerate(60.0f);
		}
	}
	else if (key == "FFmpeg_ShouldRecordUI")
	{
		g_pState->Infrastructure->FFmpeg->SetRecordUI(value == "1" || value == "true");
	}
	else if (key == "FFmpeg_OutputPath")
	{
		g_pState->Infrastructure->FFmpeg->SetOutputPath(value);
	}
	else if (key == "FFmpeg_StopOnLastEvent")
	{
		g_pState->Infrastructure->FFmpeg->SetStopOnLastEvent(value == "1" || value == "true");
	}
	else if (key == "FFmpeg_StopDelayDuration")
	{
		try
		{
			g_pState->Infrastructure->FFmpeg->SetStopDelayDuration(std::stof(value));
		}
		catch (...)
		{
			g_pState->Infrastructure->FFmpeg->SetStopDelayDuration(0.0f);
		}
	}
	else if (key == "FFmpeg_ThreadQueueSize")
	{
		encoderConfig.ThreadQueueSize = std::stoi(value);
	}
	else if (key == "FFmpeg_BitrateKbps")
	{
		encoderConfig.BitrateKbps = std::stoi(value);
	}
	else if (key == "FFmpeg_VideoBufferPipeSize")
	{
		encoderConfig.VideoBufferPipeSize = std::stoi(value);
	}
	else if (key == "FFmpeg_VideoPreset")
	{
		encoderConfig.VideoPreset = static_cast<VideoPreset>(std::stoi(value));
	}
	else if (key == "FFmpeg_ScalingFilter")
	{
		encoderConfig.ScalingFilter = static_cast<ScalingFilter>(std::stoi(value));
	}
	else if (key == "FFmpeg_OutputContainer")
	{
		encoderConfig.OutputContainer = static_cast<OutputContainer>(std::stoi(value));
	}
	else if (key == "FFmpeg_EncoderType")
	{
		encoderConfig.EncoderType = static_cast<EncoderType>(std::stoi(value));
	}

	g_pState->Infrastructure->FFmpeg->UpdateEncoderConfig(encoderConfig);
}

void PreferencesSystem::LoadLifecycleState(std::string& key, std::string& value)
{
	if (key == "Lifecycle_AutoUpdatePhase")
	{
		g_pState->Infrastructure->Lifecycle->SetAutoUpdatePhase(value == "1" || value == "true");
	}
}

void PreferencesSystem::LoadSettingsState(std::string& key, std::string& value)
{
	if (key == "Settings_ShouldFreezeMouse")
	{
		g_pState->Infrastructure->Settings->SetFreezeMouse(value == "1" || value == "true");
	}
	else if (key == "Settings_ReplaySpeedModifierKeys")
	{
		g_pState->Infrastructure->Settings->SetUseManualInput(value == "1" || value == "true");
	}
	else if (key == "Settings_MenuAlpha")
	{
		try
		{
			g_pState->Infrastructure->Settings->SetMenuAlpha(std::stof(value));
		}
		catch (...)
		{
			g_pState->Infrastructure->Settings->SetMenuAlpha(1.0f);
		}
	}
	else if (key == "Settings_UIScale")
	{
		try
		{
			g_pState->Infrastructure->Render->SetUIScale(std::stof(value));
		}
		catch (...)
		{
			g_pState->Infrastructure->Render->SetUIScale(1.0f);
		}
	}
	else if (key == "Settings_PreferredPhase")
	{
		try
		{
			int phaseInt = std::stoi(value);

			if (phaseInt >= 0 && phaseInt <= 2)
			{
				g_pState->Infrastructure->Settings->SetPreferredPhase(static_cast<Phase>(phaseInt));
			}
			else
			{
				g_pState->Infrastructure->Settings->SetPreferredPhase(Phase::Default);
			}
		}
		catch (...)
		{
			g_pState->Infrastructure->Settings->SetPreferredPhase(Phase::Default);
		}
	}
}

void PreferencesSystem::LoadUI(std::string& key, std::string& value)
{
	if (key == "UI_TimelineAutoScroll")
	{
		g_pState->Infrastructure->Settings->SetTimelineAutoScroll(value == "1" || value == "true");
	}
	else if (key == "UI_TheaterAutoScroll")
	{
		g_pState->Infrastructure->Settings->SetTheaterAutoScroll(value == "1" || value == "true");
	}
	else if (key == "UI_DirectorAutoScroll")
	{
		g_pState->Infrastructure->Settings->SetDirectorAutoScroll(value == "1" || value == "true");
	}
	else if (key == "UI_LogsAutoScroll")
	{
		g_pState->Infrastructure->Settings->SetLogsAutoScroll(value == "1" || value == "true");
	}
	else if (key == "UI_LockThirdPersonPOV")
	{
		g_pState->Domain->Theater->SetThirdPersonForced(value == "1" || value == "true");
	}
}