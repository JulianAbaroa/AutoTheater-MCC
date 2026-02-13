#pragma once

#include "Core/Common/Types/UserInterfaceTypes.h"
#include <string>
#include <vector>

class ReplaySystem
{
public:
	void SaveReplay(const std::string& sourceFilmPath);
	void DeleteReplay(const std::string& hash);

	void SaveMetadata(const std::string& hash, const std::string& customName);
	void RenameReplay(const std::string& hash, const std::string& newName);
	void RestoreReplay(const SavedReplay& replay);

	void SaveTimeline(const std::string& replayName);
	void LoadTimeline(const std::string& replayName);

	std::vector<SavedReplay> GetSavedReplays();
	std::string CalculateFileHash(const std::string& sourceFilmPath);

private:
	float GetLastTimestampFromFile(const std::string& timelinePath);
};