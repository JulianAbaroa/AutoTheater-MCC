#pragma once

class SettingsSystem
{
public:	
	void InitializePaths();

	void SaveUseAppData();
	void LoadUseAppData();

	void CreateAppData();
	void DeleteAppData();
};