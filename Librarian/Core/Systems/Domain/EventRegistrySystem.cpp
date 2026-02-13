#include "pch.h"
#include "Utils/Formatting.h"
#include "Core/Common/AppCore.h"
#include "EventRegistrySystem.h"
#include <fstream>
#include <string>

void EventRegistrySystem::SaveEventRegistry()
{
	std::string path = g_pState->Settings.GetAppDataDirectory() + "\\event_weights.cfg";;
	auto eventRegistryCopy = g_pState->EventRegistry.GetEventRegistryCopy();

	std::ofstream file(path);
	if (!file.is_open()) return;

	file << "# AutoTheater Event Weights\n";
	for (const auto& [name, info] : eventRegistryCopy)
	{
		file << Formatting::WStringToString(name) << "=" << info.Weight << "\n";
	}

	file.close();
}

void EventRegistrySystem::LoadEventRegistry()
{
	std::string path = g_pState->Settings.GetAppDataDirectory() + "\\event_weights.cfg";
	auto eventRegistryCopy = g_pState->EventRegistry.GetEventRegistryCopy();;

	std::ifstream file(path);
	if (!file.is_open()) return;

	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#') continue;

		size_t delimiterPos = line.find_last_of('=');
		if (delimiterPos != std::string::npos)
		{
			std::string nameStr = line.substr(0, delimiterPos);
			std::string weightStr = line.substr(delimiterPos + 1);

			std::wstring eventName = Formatting::StringToWString(nameStr);

			if (eventRegistryCopy.count(eventName))
			{
				try {
					eventRegistryCopy[eventName].Weight = std::stoi(weightStr);
				}
				catch (...) {}
			}
		}
	}

	file.close();

	g_pState->EventRegistry.SetEventRegistry(eventRegistryCopy);
}