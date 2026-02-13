#include "pch.h"
#include "Core/States/Infrastructure/SettingsState.h"

bool SettingsState::ShouldUseAppData() const
{ 
	return m_UseAppData.load(); 
}

bool SettingsState::IsMenuVisible() const
{ 
	return m_IsMenuVisible.load(); 
}

bool SettingsState::MustResetMenu() const
{
	return m_MustResetMenu.load();
}

bool SettingsState::ShouldFreezeMouse() const
{
	return m_ShouldFreezeMouse.load();
}


void SettingsState::SetUseAppData(bool value)
{
	m_UseAppData.store(value);
}

void SettingsState::SetMenuVisible(bool value) 
{ 
	m_IsMenuVisible.store(value); 
}

void SettingsState::SetForceMenuReset(bool value) 
{ 
	m_MustResetMenu.store(value); 
}

void SettingsState::SetFreezeMouse(bool value) 
{ 
	m_ShouldFreezeMouse.store(value); 
}


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