#pragma once

class SettingsSystem
{
public:	
	void InitializePaths();
	void SavePreferences();
	void LoadPreferences();

	void CreateAppData();
	void DeleteAppData();
};