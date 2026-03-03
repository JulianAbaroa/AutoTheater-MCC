#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include <fstream>

void ReplaySystem::SaveReplay(const std::string& sourceFilmPath)
{
	if (sourceFilmPath.empty() || g_pState->Settings.IsAppDataDirectoryEmpty()) return;

	try
	{
		std::filesystem::path src(sourceFilmPath);
		if (!std::filesystem::exists(src)) return;

		TheaterReplay scannedData = this->ScanReplay(sourceFilmPath);

		std::string fileHash = CalculateFileHash(sourceFilmPath);

		std::filesystem::path destDir =
			std::filesystem::path(g_pState->Settings.GetAppDataDirectory()) / "Replays" / fileHash;

		if (!std::filesystem::exists(destDir))
			std::filesystem::create_directories(destDir);

		std::filesystem::path finalMovPath = destDir / (fileHash + ".mov");

		std::filesystem::copy_file(
			src, finalMovPath,
			std::filesystem::copy_options::overwrite_existing
		);

		this->SaveMetadata(fileHash, src.stem().string(), scannedData.FilmMetadata);

		g_pState->Replay.SetPreviousReplayHash(fileHash);
		g_pState->Replay.SetRefreshReplayList(true);

		g_pUtil->Log.Append("[ReplaySystem] INFO: Replay saved & indexed: %s", fileHash.substr(0, 8).c_str());
	}
	catch (const std::exception& e)
	{
		g_pUtil->Log.Append("[ReplaySystem] ERROR: On saving replay, message: %s", e.what());
	}
}

void ReplaySystem::DeleteReplay(const std::string& hash)
{
	try
	{
		std::filesystem::path dir =
			std::filesystem::path(g_pState->Settings.GetAppDataDirectory()) / "Replays" / hash;

		if (std::filesystem::exists(dir))
		{
			std::filesystem::remove_all(dir);
			g_pState->Replay.SetRefreshReplayList(true);
			g_pUtil->Log.Append("[ReplaySystem] INFO: Replay deleted: %s", hash);
		}
	}
	catch (const std::exception& e)
	{
		g_pUtil->Log.Append("[ReplaySystem] ERROR: On deleting replay: %s", e.what());
	}
}


void ReplaySystem::SaveMetadata(const std::string& hash, const std::string& defaultName, const FilmMetadata& metadata)
{
	std::filesystem::path path =
		std::filesystem::path(g_pState->Settings.GetAppDataDirectory()) / "Replays" / hash / "metadata.txt";

	std::string finalName = defaultName;
	std::string localHash = hash;

	std::string author = metadata.Author.empty() ? "Unknown" : metadata.Author;
	std::string info = metadata.Info.empty() ? "N/A" : metadata.Info;

	if (std::filesystem::exists(path)) 
	{
		std::ifstream inFile(path);
		std::string line;

		while (std::getline(inFile, line)) 
		{
			if (line.find("Name=") == 0) finalName = line.substr(5);
		}

		inFile.close();
	}

	std::ofstream file(path, std::ios::trunc);
	if (file.is_open()) 
	{
		file << "Name=" << finalName << "\n";
		file << "Hash=" << localHash << "\n";
		file << "Author=" << author << "\n";
		file << "Info=" << info << "\n";
		file.close();
	}
}

void ReplaySystem::RenameReplay(const std::string& hash, const std::string& newName)
{
	std::filesystem::path metaPath;
	std::string author = "Unknown", info = "N/A";
	metaPath =
		std::filesystem::path(g_pState->Settings.GetAppDataDirectory()) / "Replays" / hash / "metadata.txt";

	if (std::filesystem::exists(metaPath)) 
	{
		std::ifstream inFile(metaPath);
		std::string line;

		while (std::getline(inFile, line)) 
		{
			if (line.find("Author=") == 0) author = line.substr(7);
			else if (line.find("Info=") == 0) info = line.substr(5);
		}
	}

	std::ofstream outFile(metaPath, std::ios::trunc);
	if (outFile.is_open()) 
	{
		outFile << "Name=" << newName << "\n";
		outFile << "Author=" << author << "\n";
		outFile << "Info=" << info << "\n";
		outFile.close();

		g_pState->Replay.SetRefreshReplayList(true);
		g_pUtil->Log.Append("[ReplaySystem] INFO: Replay renamed to: %s", newName);
	}
}

void ReplaySystem::RestoreReplay(const SavedReplay& replay)
{
	if (g_pState->Settings.IsMovieTempDirectoryEmpty()) return;

	try
	{
		std::filesystem::path src = replay.TheaterReplay.FullPath / replay.TheaterReplay.MovFileName;
		std::filesystem::path dest = std::filesystem::path(g_pState->Settings.GetMovieTempDirectory()) / replay.TheaterReplay.MovFileName;

		if (std::filesystem::exists(dest))
		{
			g_pUtil->Log.Append("[ReplaySystem] WARNING: File already exists in MCC folder, overwriting.");
		}

		std::filesystem::copy_file(
			src, dest,
			std::filesystem::copy_options::overwrite_existing
		);

		g_pUtil->Log.Append("[ReplaySystem] INFO: Replay sent to MCC Temp folder, you can now open it in-game.");
	}
	catch (const std::exception& e)
	{
		g_pUtil->Log.Append("[ReplaySystem] ERROR: Restore failed %s.", e.what());
	}
}


void ReplaySystem::SaveTimeline(const std::string& replayHash)
{
	if (!g_pState->Settings.ShouldUseAppData() || g_pState->Settings.IsAppDataDirectoryEmpty()) return;

	std::filesystem::path timelinePath =
		std::filesystem::path(g_pState->Settings.GetAppDataDirectory()) / "Replays" / replayHash / "events.timeline";

	if (std::filesystem::exists(timelinePath))
	{
		float existingLastTimestamp = this->GetLastTimestampFromFile(timelinePath.string());
		float currentTimestamp = g_pSystem->Timeline.GetLatestTimestamp();

		if (currentTimestamp <= existingLastTimestamp)
		{
			g_pUtil->Log.Append("[ReplaySystem] WARNING: Existing timeline is more complete, skipping.");
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

	std::vector<GameEvent> timelineCopy = g_pState->Timeline.GetTimelineCopy();;

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
	g_pState->Replay.SetRefreshReplayList(true);
	g_pUtil->Log.Append("[ReplaySystem] INFO: Timeline synchronized successfully.");
}

void ReplaySystem::LoadTimeline(const std::string& hash)
{
	std::filesystem::path path =
		std::filesystem::path(g_pState->Settings.GetAppDataDirectory()) / "Replays" / hash / "events.timeline";

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) return;

	try
	{
		auto loadString = [&](std::string& s) {
			size_t len = 0;
			if (!file.read((char*)&len, sizeof(len))) return;

			if (len > 0 && len < 2048) 
			{
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

		for (size_t i = 0; i < eventCount; i++) 
		{
			GameEvent gameEvent;
			file.read((char*)&gameEvent.Timestamp, sizeof(gameEvent.Timestamp));
			file.read((char*)&gameEvent.Type, sizeof(gameEvent.Type));
			file.read((char*)&gameEvent.Teams, sizeof(gameEvent.Teams));

			size_t playerCount = 0;
			file.read((char*)&playerCount, sizeof(playerCount));

			for (size_t j = 0; j < playerCount; j++) 
			{
				PlayerInfo p;
				file.read((char*)&p.Id, sizeof(p.Id));
				loadString(p.Name);
				loadString(p.Tag);

				file.read((char*)&p.RawPlayer, sizeof(RawPlayer));

				size_t weaponCount = 0;
				file.read((char*)&weaponCount, sizeof(weaponCount));

				if (weaponCount > 0 && weaponCount < 10) 
				{
					p.Weapons.resize(weaponCount);

					for (size_t k = 0; k < weaponCount; k++) 
					{
						file.read((char*)&p.Weapons[k], sizeof(RawWeapon));
					}
				}
				gameEvent.Players.push_back(p);
			}
			tempTimeline.push_back(std::move(gameEvent));
		}

		g_pState->Timeline.SetTimeline(tempTimeline);
		g_pState->Replay.SetPreviousReplayHash(hash);

		g_pUtil->Log.Append("[ReplaySystem] INFO: Timeline loaded, active hash set to: %s", hash.c_str());
	}
	catch (const std::exception& e) 
	{
		g_pUtil->Log.Append("[ReplaySystem] WARNING: Crash prevented in LoadTimeline: %s", e.what());
	}

	file.close();
}


std::vector<SavedReplay> ReplaySystem::GetSavedReplays()
{
	std::vector<SavedReplay> replays;
	std::string appDataDirectory = g_pState->Settings.GetAppDataDirectory();
	std::filesystem::path root = std::filesystem::path(appDataDirectory) / "Replays";

	if (!std::filesystem::exists(root)) return replays;

	for (const auto& entry : std::filesystem::directory_iterator(root))
	{
		if (entry.is_directory())
		{
			SavedReplay replay;
			replay.Hash = entry.path().filename().string();
			replay.TheaterReplay.FullPath = entry.path();
			replay.HasTimeline = std::filesystem::exists(entry.path() / "events.timeline");

			std::filesystem::path metaPath = entry.path() / "metadata.txt";
			if (std::filesystem::exists(metaPath))
			{
				std::ifstream file(metaPath);
				std::string line;
				while (std::getline(file, line))
				{
					if (line.find("Name=") == 0) replay.DisplayName = line.substr(5);
					else if (line.find("Author=") == 0) replay.TheaterReplay.FilmMetadata.Author = line.substr(7);
					else if (line.find("Info=") == 0) replay.TheaterReplay.FilmMetadata.Info = line.substr(5);
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
					replay.TheaterReplay.MovFileName = f.path().filename().string();
					break;
				}
			}

			replays.push_back(replay);
		}
	}

	return replays;
}

std::string ReplaySystem::CalculateFileHash(const std::string& sourceFilmPath)
{
	std::ifstream file(sourceFilmPath, std::ios::binary);
	if (!file) return "default_hash";

	file.seekg(0, std::ios::end);
	size_t fileSize = file.tellg();

	std::vector<char> fileData;
	const size_t chunkSize = 4096;

	if (fileSize <= chunkSize * 3) 
	{
		fileData.resize(fileSize);
		file.seekg(0, std::ios::beg);
		file.read(fileData.data(), fileSize);
	}
	else 
	{
		fileData.resize(chunkSize * 3);

		file.seekg(0, std::ios::beg);
		file.read(fileData.data(), chunkSize);

		file.seekg(fileSize / 2, std::ios::beg);
		file.read(fileData.data() + chunkSize, chunkSize);

		file.seekg(fileSize - chunkSize, std::ios::beg);
		file.read(fileData.data() + (chunkSize * 2), chunkSize);
	}

	// FNV-1a Algorithm (64-bit)
	uint64_t hash = 0xcbf29ce484222325;
	for (char c : fileData)
	{
		hash ^= static_cast<uint8_t>(c);
		hash *= 0x100000001b3;
	}

	char buffer[32]{};
	snprintf(buffer, sizeof(buffer), "%016llx", hash);
	return std::string(buffer);
}

std::vector<TheaterReplay> ReplaySystem::GetTheaterReplays(const std::filesystem::path& directoryPath)
{
	std::vector<TheaterReplay> replays;

	if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath))
	{
		return replays;
	}

	for (const auto& entry : std::filesystem::directory_iterator(directoryPath))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".mov")
		{
			replays.push_back(this->ScanReplay(entry.path()));
		}
	}

	return replays;
}

TheaterReplay ReplaySystem::ScanReplay(const std::filesystem::path& filePath)
{
	TheaterReplay replay;
	replay.FullPath = filePath;
	replay.MovFileName = filePath.filename().string();

	std::ifstream file(filePath, std::ios::binary);
	if (file.is_open())
	{
		char buffer[0x400] = { 0 };
		file.read(buffer, sizeof(buffer));
		file.close();

		char author[17] = { 0 };
		memcpy(author, buffer + 0x88, 16);

		wchar_t* wFullInfo = reinterpret_cast<wchar_t*>(buffer + 0x1C0);

		replay.FilmMetadata.Author = std::string(author);
		replay.FilmMetadata.Info = g_pUtil->Format.WStringToString(wFullInfo);
	}

	return replay;
}

void ReplaySystem::DeleteInGameReplay(const std::filesystem::path& replayPath)
{
	try
	{
		if (std::filesystem::exists(replayPath))
		{
			std::filesystem::remove(replayPath);
			g_pUtil->Log.Append("[ReplaySystem] INFO: Deleted in-game replay: %s",
				replayPath.filename().string().c_str());
			this->HotreloadReplays();
		}
	}
	catch (const std::exception& e)
	{
		g_pUtil->Log.Append("[ReplaySystem] ERROR: On deleting in-game replay: %s", e.what());
	}
}


float ReplaySystem::GetLastTimestampFromFile(const std::string& timelinePath)
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

void ReplaySystem::HotreloadReplays()
{
	try
	{
		std::filesystem::path dir = std::filesystem::path(g_pState->Settings.GetMovieTempDirectory());

		if (std::filesystem::exists(dir))
		{
			std::filesystem::path tempFile = dir / ".hotreload_trigger.mov";

			if (std::filesystem::exists(tempFile))
			{
				std::filesystem::remove(tempFile);
			}

			std::ofstream file(tempFile);
			file.close();

			g_pUtil->Log.Append("[ReplaySystem] INFO: Replays hotreload executed.");
		}
	}
	catch (const std::exception& e)
	{
		g_pUtil->Log.Append("[ReplaySystem] ERROR: In hotreload replays. %s.", e.what());
	}
}