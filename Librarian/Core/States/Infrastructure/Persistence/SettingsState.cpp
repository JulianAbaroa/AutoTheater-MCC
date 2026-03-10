#include "pch.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"

bool SettingsState::ShouldUseAppData() const { return m_UseAppData.load(); }
bool SettingsState::IsMenuVisible() const { return m_IsMenuVisible.load(); }
bool SettingsState::MustResetMenu() const { return m_MustResetMenu.load(); }
bool SettingsState::ShouldFreezeMouse() const { return m_ShouldFreezeMouse.load(); }
bool SettingsState::ShouldUseManualInput() const { return m_ShouldUseManualInput.load(); }
float SettingsState::GetMenuAlpha() const { return m_MenuAlpha.load(); }
bool SettingsState::GetTimelineAutoScroll() const { return m_TimelineAutoScroll.load(); }
bool SettingsState::GetTheaterAutoScroll() const { return m_TheaterAutoScroll.load(); }
bool SettingsState::GetDirectorAutoScroll() const { return m_DirectorAutoScroll.load(); }
bool SettingsState::GetLogsAutoScroll() const { return m_LogsAutoScroll.load(); }
Phase SettingsState::GetPreferredPhase() const { return m_PreferredPhase.load(); }

void SettingsState::SetUseAppData(bool value) { m_UseAppData.store(value); }
void SettingsState::SetMenuVisible(bool value) { m_IsMenuVisible.store(value); }
void SettingsState::SetForceMenuReset(bool value) { m_MustResetMenu.store(value); }
void SettingsState::SetFreezeMouse(bool value) { m_ShouldFreezeMouse.store(value); }
void SettingsState::SetUseManualInput(bool value) { m_ShouldUseManualInput.store(value); }
void SettingsState::SetMenuAlpha(float value) { m_MenuAlpha.store(value); }
void SettingsState::SetTimelineAutoScroll(bool value) { m_TimelineAutoScroll.store(value); }
void SettingsState::SetTheaterAutoScroll(bool value) { m_TheaterAutoScroll.store(value); }
void SettingsState::SetDirectorAutoScroll(bool value) { m_DirectorAutoScroll.store(value); }
void SettingsState::SetLogsAutoScroll(bool value) { m_LogsAutoScroll.store(value); }
void SettingsState::SetPreferredPhase(Phase phase) { m_PreferredPhase.store(phase); }

std::string SettingsState::GetBaseDirectory() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_BaseDirectory;
}

std::string SettingsState::GetAppDataDirectory() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_AppDataDirectory;
}

std::string SettingsState::GetMovieTempDirectory() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_MovieTempDirectory;
}

std::string SettingsState::GetLoggerPath() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_LoggerPath;
}


void SettingsState::SetBaseDirectory(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_BaseDirectory = directory;
}

void SettingsState::SetAppDataDirectory(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_AppDataDirectory = directory;
}

void SettingsState::SetMovieTempDirectory(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_MovieTempDirectory = directory;
}

void SettingsState::SetLoggerPath(const std::string& directory)
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_LoggerPath = directory;
}


void SettingsState::ClearAppDataDirectory()
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	m_AppDataDirectory.clear();
}

bool SettingsState::IsAppDataDirectoryEmpty() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_AppDataDirectory.empty();
}

bool SettingsState::IsMovieTempDirectoryEmpty() const
{
	std::lock_guard<std::mutex> lock(m_Mutex);
	return m_MovieTempDirectory.empty();
}