#include "pch.h"
#include "Utils/Logger.h"
#include "Utils/Formatting.h"
#include "Core/Common/GlobalState.h"
#include "Core/Common/PersistenceManager.h"
#include <shlobj.h>
#include <fstream>

void PersistenceManager::InitializePaths()
{
	PWSTR localLowPath = NULL;
	if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppDataLow, 0, NULL, &localLowPath)))
	{
		std::filesystem::path base(localLowPath);

		g_pState->MCCTempMovieDirectory = (base / "MCC/Temporary/UserContent/HaloReach/Movie").string();
		CoTaskMemFree(localLowPath);
	}

	LoadPreferences();

	if (g_pState->UseAppData.load())
	{
		CreateAppData();
	}
}

void PersistenceManager::SavePreferences()
{
	std::string configPath = g_pState->BaseDirectory + "\\config.ini";
	std::ofstream file(configPath);
	if (file.is_open())
	{
		file << "useAppData=" << (g_pState->UseAppData.load() ? "1" : "0") << "\n";
		file.close();
	}
}

void PersistenceManager::LoadPreferences()
{
	g_pState->UseAppData.store(false);

	std::string configPath = g_pState->BaseDirectory + "\\config.ini";
	std::ifstream file(configPath);
	if (file.is_open())
	{
		std::string line;
		while (std::getline(file, line))
		{
			if (line.find("useAppData=1") != std::string::npos)
			{
				g_pState->UseAppData.store(true);
			}
		}

		file.close();
	}
}

void PersistenceManager::CreateAppData()
{
	if (!g_pState->UseAppData.load() ||
		!g_pState->AppDataDirectory.empty()) return;

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
			g_pState->AppDataDirectory = basePath.string();
		}

		CoTaskMemFree(pathTemp);
	}
}

void PersistenceManager::DeleteAppData()
{
	if (g_pState->AppDataDirectory.empty()) return;

	std::error_code errorCode;

	if (std::filesystem::remove_all(g_pState->AppDataDirectory, errorCode) > 0)
	{
		g_pState->AppDataDirectory.clear();
	}

	if (errorCode)
	{
		std::string msg = "ERROR: While deleting AppData, " + errorCode.message();
		Logger::LogAppend(msg.c_str());
	}
}



std::string PersistenceManager::CalculateFileHash(const std::string& sourceFilmPath)
{
	std::ifstream file(sourceFilmPath, std::ios::binary);
	if (!file) return "default_hash";

	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();

	std::vector<char> buffer;
	const size_t chunkSize = 4096;

	if (fileSize <= chunkSize * 3) {
		buffer.resize(fileSize);
		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), fileSize);
	}
	else {
		buffer.resize(chunkSize * 3);

		file.seekg(0, std::ios::beg);
		file.read(buffer.data(), chunkSize);

		file.seekg(fileSize / 2, std::ios::beg);
		file.read(buffer.data() + chunkSize, chunkSize);

		file.seekg(fileSize - chunkSize, std::ios::beg);
		file.read(buffer.data() + (chunkSize * 2), chunkSize);
	}

	// FNV-1a Algorithm (64-bit)
	uint64_t hash = 0xcbf29ce484222325;
	for (char c : buffer)
	{
		hash ^= static_cast<uint8_t>(c);
		hash *= 0x100000001b3;
	}

	std::stringstream ss;
	ss << std::hex << std::setw(16) << std::setfill('0') << hash;
	return ss.str();
}

void PersistenceManager::SaveReplay(const std::string& sourceFilmPath)
{
	if (sourceFilmPath.empty() || g_pState->AppDataDirectory.empty()) return;

	try
	{
		std::filesystem::path src(sourceFilmPath);
		if (!std::filesystem::exists(src)) return;

		std::string fileHash = CalculateFileHash(sourceFilmPath);

		std::filesystem::path destDir;
		{
			std::lock_guard lock(g_pState->ConfigMutex);
			destDir = std::filesystem::path(g_pState->AppDataDirectory) / "Replays" / fileHash;
		}

		std::filesystem::create_directories(destDir);

		std::filesystem::path finalMovPath = destDir / src.filename();
		std::filesystem::copy_file(
			src, finalMovPath,
			std::filesystem::copy_options::overwrite_existing
		);

		SaveMetadata(fileHash, fileHash);

		{
			std::lock_guard lock(g_pState->ReplayManagerMutex);
			g_pState->ActiveReplayHash = fileHash;
		}

		g_pState->RefreshReplayList.store(true);
		Logger::LogAppend(("Replay indexed by hash: " + fileHash.substr(0, 8) + "...").c_str());
	}
	catch (const std::exception& e)
	{
		Logger::LogAppend((std::string("Error saving replay: ") + e.what()).c_str());
	}
}

void PersistenceManager::DeleteReplay(const std::string& hash)
{
	try
	{
		std::filesystem::path dir;
		{
			std::lock_guard lock(g_pState->ConfigMutex);
			dir = std::filesystem::path(g_pState->AppDataDirectory) / "Replays" / hash;
		}
			
		if (std::filesystem::exists(dir))
		{
			std::filesystem::remove_all(dir);
			g_pState->RefreshReplayList.store(true);
			Logger::LogAppend(("Replay deleted: " + hash).c_str());
		}
	}
	catch (const std::exception& e)
	{
		Logger::LogAppend((std::string("Error deleting replay: ") + e.what()).c_str());
	}
}



void PersistenceManager::SaveMetadata(const std::string& hash, const std::string& defaultName)
{
	std::filesystem::path path;
	{
		std::lock_guard lock(g_pState->ConfigMutex);
		path = std::filesystem::path(g_pState->AppDataDirectory) / "Replays" / hash / "metadata.txt";
	}

	std::string finalName = defaultName;
	std::string localHash = hash;
	std::string author = "Unknown";
	std::string info = "N/A";
	
	if (std::filesystem::exists(path)) {
		std::ifstream inFile(path);
		std::string line;
		while (std::getline(inFile, line)) {
			if (line.find("Name=") == 0) finalName = line.substr(5);
			else if (line.find("Hash=") == 0) localHash = line.substr(5);
			else if (line.find("Author=") == 0) author = line.substr(7);
			else if (line.find("Info=") == 0) info = line.substr(5);
		}
		inFile.close();
	}
	else {
		std::lock_guard lock(g_pState->ReplayManagerMutex);
		author = g_pState->CurrentMetadata.Author;
		info = g_pState->CurrentMetadata.Info;
	}

	std::ofstream file(path, std::ios::trunc);
	if (file.is_open()) {
		file << "Name=" << finalName << "\n";
		file << "Hash=" << localHash << "\n";
		file << "Author=" << author << "\n";
		file << "Info=" << info << "\n";
		file.close();
	}
}



std::vector<SavedReplay> PersistenceManager::GetSavedReplays()
{
	std::vector<SavedReplay> replays;
	std::filesystem::path root = std::filesystem::path(g_pState->AppDataDirectory) / "Replays";

	if (!std::filesystem::exists(root)) return replays;

	for (const auto& entry : std::filesystem::directory_iterator(root))
	{
		if (entry.is_directory())
		{
			SavedReplay replay;
			replay.Hash = entry.path().filename().string();
			replay.FullPath = entry.path();
			replay.HasTimeline = std::filesystem::exists(entry.path() / "events.timeline");

			std::filesystem::path metaPath = entry.path() / "metadata.txt";
			if (std::filesystem::exists(metaPath))
			{
				std::ifstream file(metaPath);
				std::string line;
				while (std::getline(file, line))
				{
					if (line.find("Name=") == 0) replay.DisplayName = line.substr(5);
					else if (line.find("Author=") == 0) replay.Author = line.substr(7);
					else if (line.find("Info=") == 0) replay.Info = line.substr(5);
				}
			}
			else
			{
				replay.DisplayName = replay.Hash;
			}

			for (const auto& f : std::filesystem::directory_iterator(entry.path()))
			{
				if (f.path().extension() == ".mov")
				{
					replay.MovFileName = f.path().filename().string();
					break;
				}
			}

			replays.push_back(replay);
		}
	}

	return replays;
}

void PersistenceManager::RestoreReplay(const SavedReplay& replay)
{
	if (g_pState->MCCTempMovieDirectory.empty()) return;

	try
	{
		std::filesystem::path src = replay.FullPath / replay.MovFileName;
		std::filesystem::path dest = std::filesystem::path(g_pState->MCCTempMovieDirectory) / replay.MovFileName;

		if (std::filesystem::exists(dest))
		{
			Logger::LogAppend("Restore: File already exists in MCC folder, overwriting...");
		}

		std::filesystem::copy_file(
			src, dest,
			std::filesystem::copy_options::overwrite_existing
		);

		Logger::LogAppend("Restore: Replay sent to MCC Temp folder. You can now open it in-game.");
	}
	catch (const std::exception& e)
	{
		Logger::LogAppend((std::string("Restore Error: ") + e.what()).c_str());
	}
}

void PersistenceManager::RenameReplay(const std::string& hash, const std::string& newName)
{
	std::filesystem::path metaPath;
	std::string author = "Unknown", info = "N/A";

	{
		std::lock_guard lock(g_pState->ConfigMutex);
		metaPath = std::filesystem::path(g_pState->AppDataDirectory) / "Replays" / hash / "metadata.txt";
	}

	if (std::filesystem::exists(metaPath)) {
		std::ifstream inFile(metaPath);
		std::string line;
		while (std::getline(inFile, line)) {
			if (line.find("Author=") == 0) author = line.substr(7);
			else if (line.find("Info=") == 0) info = line.substr(5);
		}
	}

	std::ofstream outFile(metaPath, std::ios::trunc);
	if (outFile.is_open()) {
		outFile << "Name=" << newName << "\n";
		outFile << "Author=" << author << "\n";
		outFile << "Info=" << info << "\n";
		outFile.close();

		g_pState->RefreshReplayList.store(true); 
		Logger::LogAppend(("Replay renamed to: " + newName).c_str());
	}
}



float GetLastTimestampFromFile(const std::string& timelinePath)
{
	std::ifstream file(timelinePath, std::ios::binary);
	if (!file) return 0.0f;

	size_t eventCount = 0;
	float lastTimestamp = 0.0f;

	file.read((char*)&eventCount, sizeof(eventCount));
	if (eventCount == 0) return 0.0f;

	file.read((char*)&lastTimestamp, sizeof(lastTimestamp));
	return lastTimestamp;
}

void PersistenceManager::SaveTimeline(const std::string& replayHash)
{
	if (!g_pState->UseAppData.load() || g_pState->AppDataDirectory.empty()) return;

	std::filesystem::path timelinePath = 
		std::filesystem::path(g_pState->AppDataDirectory) / "Replays" / replayHash / "events.timeline";

	if (std::filesystem::exists(timelinePath))
	{
		float existingLastTimestamp = GetLastTimestampFromFile(timelinePath.string());

		float currentTimestamp = 0.0f;
		{
			std::lock_guard lock(g_pState->TimelineMutex);
			if (!g_pState->Timeline.empty())
			{
				currentTimestamp = g_pState->Timeline.back().Timestamp;
			}
		}

		if (currentTimestamp <= existingLastTimestamp)
		{
			Logger::LogAppend("SaveTimeline: Existing timeline is more complete. Skipping...");
			return;
		}
	}

	std::ofstream file(timelinePath, std::ios::binary);
	if (!file.is_open()) return;

	auto saveString = [&](const std::string& s) {
		size_t len = s.length();
		file.write((char*)&len, sizeof(len));
		file.write(s.c_str(), len);
	};

	std::vector<GameEvent> timelineCopy;

	{
		std::lock_guard lock(g_pState->TimelineMutex);
		timelineCopy = g_pState->Timeline;
	}

	size_t eventCount = timelineCopy.size();
	float lastTimestamp = timelineCopy.empty() ? 0.0f : timelineCopy.back().Timestamp;

	file.write((char*)&eventCount, sizeof(eventCount));
	file.write((char*)&lastTimestamp, sizeof(lastTimestamp));

	for (const auto& ev : timelineCopy)
	{
		file.write((char*)&ev.Timestamp, sizeof(ev.Timestamp));
		file.write((char*)&ev.Type, sizeof(ev.Type));

		file.write((char*)&ev.Teams, sizeof(ev.Teams));

		size_t playerCount = ev.Players.size();
		file.write((char*)&playerCount, sizeof(playerCount));

		for (const auto& p : ev.Players)
		{
			file.write((char*)&p.Id, sizeof(p.Id));

			saveString(p.Name);
			saveString(p.Tag);

			file.write((char*)&p.RawPlayer, sizeof(RawPlayer));

			size_t weaponCount = p.Weapons.size();
			file.write((char*)&weaponCount, sizeof(weaponCount));

			for (const auto& w : p.Weapons)
			{
				file.write((char*)&w, sizeof(RawWeapon));
			}
		}
	}

	file.close();
	g_pState->RefreshReplayList.store(true);
	Logger::LogAppend("Timeline synchronized successfully.");
}

void PersistenceManager::LoadTimeline(const std::string& hash)
{
	std::filesystem::path path;
	{
		std::lock_guard lock(g_pState->ConfigMutex);
		path = std::filesystem::path(g_pState->AppDataDirectory) / "Replays" / hash / "events.timeline";
	}

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) return;

	try
	{
		auto loadString = [&](std::string& s) {
			size_t len = 0;
			if (!file.read((char*)&len, sizeof(len))) return;

			if (len > 0 && len < 2048) {
				s.resize(len);
				file.read(&s[0], len);
			}
		};

		size_t eventCount = 0;
		float lastTimestampHeader = 0.0f;

		file.read((char*)&eventCount, sizeof(eventCount));
		file.read((char*)&lastTimestampHeader, sizeof(lastTimestampHeader));

		if (eventCount > 100000) eventCount = 100000;

		std::vector<GameEvent> tempTimeline;
		tempTimeline.reserve(eventCount);

		for (size_t i = 0; i < eventCount; i++) {
			GameEvent gameEvent;
			file.read((char*)&gameEvent.Timestamp, sizeof(gameEvent.Timestamp));
			file.read((char*)&gameEvent.Type, sizeof(gameEvent.Type));
			file.read((char*)&gameEvent.Teams, sizeof(gameEvent.Teams));

			size_t playerCount = 0;
			file.read((char*)&playerCount, sizeof(playerCount));

			for (size_t j = 0; j < playerCount; j++) {
				PlayerInfo p;
				file.read((char*)&p.Id, sizeof(p.Id));
				loadString(p.Name);
				loadString(p.Tag);

				file.read((char*)&p.RawPlayer, sizeof(RawPlayer));

				size_t weaponCount = 0;
				file.read((char*)&weaponCount, sizeof(weaponCount));

				if (weaponCount > 0 && weaponCount < 10) {
					p.Weapons.resize(weaponCount);
					for (size_t k = 0; k < weaponCount; k++) {
						file.read((char*)&p.Weapons[k], sizeof(RawWeapon));
					}
				}
				gameEvent.Players.push_back(p);
			}
			tempTimeline.push_back(std::move(gameEvent));
		}

		{
			std::lock_guard lock(g_pState->TimelineMutex);
			g_pState->Timeline = std::move(tempTimeline);
		}

		{
			std::lock_guard lock(g_pState->ReplayManagerMutex);
			g_pState->ActiveReplayHash = hash;
		}

		std::stringstream ss;
		ss << "Timeline loaded. Active Hash set to: " << hash.c_str();
		Logger::LogAppend(ss.str().c_str());
	}
	catch (const std::exception& e) {
		Logger::LogAppend((std::string("Crash prevented in LoadTimeline: ") + e.what()).c_str());
	}

	file.close();
}


void PersistenceManager::SaveEventRegistry()
{
	std::string path;
	std::unordered_map<std::wstring, EventInfo> eventRegistryCopy;

	{
		std::lock_guard<std::mutex> lockConfig(g_pState->ConfigMutex);
		path = g_pState->AppDataDirectory + "\\event_weights.cfg";
		eventRegistryCopy = g_pState->EventRegistry;
	}

	std::ofstream file(path);
	if (!file.is_open()) return;

	file << "# AutoTheater Event Weights\n";
	for (const auto& [name, info] : eventRegistryCopy)
	{
		file << Formatting::WStringToString(name) << "=" << info.Weight << "\n";
	}

	file.close();
}

void PersistenceManager::LoadEventRegistry()
{
	std::string path;
	std::unordered_map<std::wstring, EventInfo> eventRegistryCopy;

	{
		std::lock_guard<std::mutex> lockConfig(g_pState->ConfigMutex);
		path = g_pState->AppDataDirectory + "\\event_weights.cfg";
		eventRegistryCopy = g_pState->EventRegistry;
	}

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
				catch (...) { }
			}
		}
	}

	{
		std::lock_guard<std::mutex> lockConfig(g_pState->ConfigMutex);
		g_pState->EventRegistry = eventRegistryCopy;
	}

	file.close();
}