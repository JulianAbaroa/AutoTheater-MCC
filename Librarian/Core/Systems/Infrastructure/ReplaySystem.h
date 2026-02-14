#pragma once

#include "Core/Common/Types/UserInterfaceTypes.h"
#include <string>
#include <vector>

class ReplaySystem
{
public:
	void SaveReplay(const std::string& sourceFilmPath);
	void DeleteReplay(const std::string& hash);

	void SaveMetadata(const std::string& hash, const std::string& defaultName, const FilmMetadata& metadata);
	void RenameReplay(const std::string& hash, const std::string& newName);
	void RestoreReplay(const SavedReplay& replay);

	void SaveTimeline(const std::string& replayName);
	void LoadTimeline(const std::string& replayName);

	std::vector<SavedReplay> GetSavedReplays();
	std::string CalculateFileHash(const std::string& sourceFilmPath);

	std::vector<TheaterReplay> GetTheaterReplays(const std::filesystem::path& directoryPath);
	TheaterReplay ScanReplay(const std::filesystem::path& filePath);
	void DeleteInGameReplay(const std::filesystem::path& replayPath);

private:
	float GetLastTimestampFromFile(const std::string& timelinePath);
	void HotreloadReplays();
};