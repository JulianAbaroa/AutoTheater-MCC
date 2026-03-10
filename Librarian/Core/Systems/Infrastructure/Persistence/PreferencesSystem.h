#pragma once

#include <string>

class PreferencesSystem
{
public:
	void SavePreferences();
	void LoadPreferences();

private:
	std::string GetPreferencesFilePath() const;
	void ParseLine(const std::string& line);

	void SaveFFmpegState(std::ofstream& file);
	void SaveLifeCycleState(std::ofstream& file);
	void SaveSettingsState(std::ofstream& file);
	void SaveUI(std::ofstream& file);

	void LoadFFmpegState(std::string& key, std::string& value);
	void LoadLifecycleState(std::string& key, std::string& value);
	void LoadSettingsState(std::string& key, std::string& value);
	void LoadUI(std::string& key, std::string& value);
};