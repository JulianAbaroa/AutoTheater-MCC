#pragma once

class SettingsSystem
{
public:	
	void InitializePaths(char* buffer);

	void SaveUseAppData();
	void LoadUseAppData();

	void CreateAppData();
	void DeleteAppData();
};