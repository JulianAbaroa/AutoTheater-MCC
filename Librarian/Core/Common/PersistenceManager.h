#pragma once

#include <string>
#include <vector>

namespace PersistenceManager
{
	void InitializePaths();
	void SavePreferences();
	void LoadPreferences();

	void CreateAppData();
	void DeleteAppData();

	std::string CalculateFileHash(const std::string& sourceFilmPath);
	void SaveReplay(const std::string& sourceFilmPath);
	void DeleteReplay(const std::string& hash);

	void SaveMetadata(const std::string& hash, const std::string& customName);
	
	std::vector<SavedReplay> GetSavedReplays();
	void RestoreReplay(const SavedReplay& replay);
	void RenameReplay(const std::string& hash, const std::string& newName);

	void SaveTimeline(const std::string& replayName);
	void LoadTimeline(const std::string& replayName);

	void SaveEventRegistry();
	void LoadEventRegistry();
}