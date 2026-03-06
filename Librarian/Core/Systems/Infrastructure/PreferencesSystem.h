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
};