#include "pch.h"
#include "Utils/Logger.h"
#include "SettingsSystem.h"
#include "Core/Common/AppCore.h"
#include <shlobj.h>
#include <fstream>

void SettingsSystem::InitializePaths()
{
	PWSTR localLowPath = NULL;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, 0, NULL, &localLowPath)))
	{
		std::filesystem::path base(localLowPath);

		g_pState->Settings.SetMovieTempDirectory((base / "MCC/Temporary/UserContent/HaloReach/Movie").string());
		CoTaskMemFree(localLowPath);
	}

	LoadPreferences();

	if (g_pState->Settings.ShouldUseAppData())
	{
		CreateAppData();
	}
}

void SettingsSystem::SavePreferences()
{
	std::string configPath = g_pState->Settings.GetBaseDirectory() + "\\config.ini";
	std::ofstream file(configPath);
	if (file.is_open())
	{
		file << "useAppData=" << (g_pState->Settings.ShouldUseAppData() ? "1" : "0") << "\n";
		file.close();
	}
}

void SettingsSystem::LoadPreferences()
{
	g_pState->Settings.SetUseAppData(false);

	std::string configPath = g_pState->Settings.GetBaseDirectory() + "\\config.ini";
	std::ifstream file(configPath);
	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			if (line.find("useAppData=1") != std::string::npos)
			{
				g_pState->Settings.SetUseAppData(true);
			}
		}

		file.close();
	}
}

void SettingsSystem::CreateAppData()
{
	if (!g_pState->Settings.ShouldUseAppData() ||
		!g_pState->Settings.IsAppDataDirectoryEmpty()) return;

	PWSTR pathTemp = NULL;

	HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pathTemp);

	if (SUCCEEDED(hr))
	{
		std::filesystem::path basePath(pathTemp);

		basePath /= "AutoTheater";

		std::error_code errorCode;
		if (std::filesystem::create_directories(basePath, errorCode) ||
			std::filesystem::exists(basePath)
			) {
			g_pState->Settings.SetAppDataDirectory(basePath.string());
		}

		CoTaskMemFree(pathTemp);
	}
}

void SettingsSystem::DeleteAppData()
{
	if (g_pState->Settings.IsAppDataDirectoryEmpty()) return;

	std::error_code errorCode;

	if (std::filesystem::remove_all(g_pState->Settings.GetAppDataDirectory(), errorCode) > 0)
	{
		g_pState->Settings.ClearAppDataDirectory();
	}

	if (errorCode)
	{
		std::string msg = "ERROR: While deleting AppData, " + errorCode.message();
		Logger::LogAppend(msg.c_str());
	}
}