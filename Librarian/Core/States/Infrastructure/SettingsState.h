#pragma once

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

	void SetUseAppData(bool value);
	void SetMenuVisible(bool value);
	void SetForceMenuReset(bool value);
	void SetFreezeMouse(bool value);

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

	std::string m_BaseDirectory{};
	std::string m_AppDataDirectory{};
	std::string m_MovieTempDirectory{};
	std::string m_LoggerPath{};

	// Mutex for thread-safe string variables modification.	
	mutable std::mutex m_Mutex;
};