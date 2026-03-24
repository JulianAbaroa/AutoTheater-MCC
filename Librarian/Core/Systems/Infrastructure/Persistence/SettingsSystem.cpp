#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/Persistence/SettingsSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <filesystem>
#include <shlobj.h>
#include <fstream>

void SettingsSystem::InitializePaths(char* buffer)
{
	g_pState->Infrastructure->Settings->SetBaseDirectory(std::string(buffer));
	g_pState->Infrastructure->Settings->SetLoggerPath(g_pState->Infrastructure->Settings->GetBaseDirectory() + "\\AutoTheater.txt");

	PWSTR localLowPath = NULL;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, 0, NULL, &localLowPath)))
	{
		std::filesystem::path base(localLowPath);
		std::filesystem::path userContent = base / "MCC" / "Temporary" / "UserContent";

		std::vector<std::string> games = { "Halo2A", "Halo3", "Halo3ODST", "Halo4", "HaloReach" };

		for (const auto& game : games)
		{
			std::filesystem::path moviePath = userContent / game / "Movie";

			g_pState->Infrastructure->Settings->AddMovieTempDirectory(moviePath.string());
		}

		CoTaskMemFree(localLowPath);
	}

	this->LoadUseAppData();

	if (g_pState->Infrastructure->Settings->ShouldUseAppData())
	{
		this->CreateAppData();
	}
}

void SettingsSystem::SaveUseAppData()
{
	std::string configPath = g_pState->Infrastructure->Settings->GetBaseDirectory() + "\\config.ini";
	std::ofstream file(configPath);
	if (file.is_open())
	{
		file << "useAppData=" << (g_pState->Infrastructure->Settings->ShouldUseAppData() ? "1" : "0") << "\n";
		file.close();
	}
}

void SettingsSystem::LoadUseAppData()
{
	g_pState->Infrastructure->Settings->SetUseAppData(false);

	std::string configPath = g_pState->Infrastructure->Settings->GetBaseDirectory() + "\\config.ini";
	std::ifstream file(configPath);
	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			if (line.find("useAppData=1") != std::string::npos)
			{
				g_pState->Infrastructure->Settings->SetUseAppData(true);
			}
		}

		file.close();
	}
}

void SettingsSystem::CreateAppData()
{
	if (!g_pState->Infrastructure->Settings->ShouldUseAppData() ||
		!g_pState->Infrastructure->Settings->IsAppDataDirectoryEmpty()) return;

	PWSTR pathTemp = NULL;

	HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &pathTemp);

	if (SUCCEEDED(hr))
	{
		std::filesystem::path basePath(pathTemp);

		basePath /= "AutoTheater";

		std::error_code errorCode;
		if (std::filesystem::create_directories(basePath, errorCode) ||
			std::filesystem::exists(basePath)) 
		{
			g_pState->Infrastructure->Settings->SetAppDataDirectory(basePath.string());
		}

		CoTaskMemFree(pathTemp);
	}
}

void SettingsSystem::DeleteAppData()
{
	if (g_pState->Infrastructure->Settings->IsAppDataDirectoryEmpty()) return;

	std::error_code errorCode;

	if (std::filesystem::remove_all(g_pState->Infrastructure->Settings->GetAppDataDirectory(), errorCode) > 0)
	{
		g_pState->Infrastructure->Settings->ClearAppDataDirectory();
	}

	if (errorCode)
	{
		g_pSystem->Debug->Log("[SettingsSystem] ERROR: While deleting AppData. %s.", errorCode.message().c_str());
	}
}