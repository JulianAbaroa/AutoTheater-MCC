#pragma once

#include "Core/Common/Types/AutoTheaterTypes.h"
#include <string>
#include <atomic>
#include <mutex>

struct SettingsState
{
public:
	bool ShouldUseAppData() const;
	bool IsMenuVisible() const;
	bool MustResetMenu() const;
	bool ShouldFreezeMouse() const;
	bool ShouldUseManualInput() const;

	void SetUseAppData(bool value);
	void SetMenuVisible(bool value);
	void SetForceMenuReset(bool value);
	void SetFreezeMouse(bool value);
	void SetUseManualInput(bool value);

	// UI
	float GetMenuAlpha() const;
	bool GetTimelineAutoScroll() const;
	bool GetTheaterAutoScroll() const;
	bool GetDirectorAutoScroll() const;
	bool GetLogsAutoScroll() const;
	AutoTheaterPhase GetPreferredPhase() const;

	void SetMenuAlpha(float value);
	void SetTimelineAutoScroll(bool value);
	void SetTheaterAutoScroll(bool value);
	void SetDirectorAutoScroll(bool value);
	void SetLogsAutoScroll(bool value);
	void SetPreferredPhase(AutoTheaterPhase phase);

	std::string GetBaseDirectory() const;
	std::string GetAppDataDirectory() const;
	std::string GetMovieTempDirectory() const;
	std::string GetLoggerPath() const;

	void SetBaseDirectory(const std::string& directory);
	void SetAppDataDirectory(const std::string& directory);
	void SetMovieTempDirectory(const std::string& directory);
	void SetLoggerPath(const std::string& directory);

	void ClearAppDataDirectory();
	bool IsAppDataDirectoryEmpty() const;
	bool IsMovieTempDirectoryEmpty() const;

private:
	std::atomic<bool> m_UseAppData{ false };
	std::atomic<bool> m_IsMenuVisible{ true };
	std::atomic<bool> m_MustResetMenu{ false };
	std::atomic<bool> m_ShouldFreezeMouse{ true };
	std::atomic<bool> m_ShouldUseManualInput{ false };

	// UI
	std::atomic<float> m_MenuAlpha{ 1.0f };
	std::atomic<bool> m_TimelineAutoScroll{ true };
	std::atomic<bool> m_TheaterAutoScroll{ true };
	std::atomic<bool> m_DirectorAutoScroll{ true };
	std::atomic<bool> m_LogsAutoScroll{ true };
	std::atomic<AutoTheaterPhase> m_PreferredPhase{ AutoTheaterPhase::Default };

	std::string m_BaseDirectory{};
	std::string m_AppDataDirectory{};
	std::string m_MovieTempDirectory{};
	std::string m_LoggerPath{};
	mutable std::mutex m_Mutex;
};