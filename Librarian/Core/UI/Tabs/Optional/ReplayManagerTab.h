#pragma once

#include "Core/States/Infrastructure/ReplayState.h"

class ReplayManagerTab
{
public: 
	void Draw();

private:
	void DrawLibrary();
	void RefreshLibraryCache();
	bool MatchesLibraryFilter(const SavedReplay& replay, const std::string& filter);
	int GetVisibleLibraryCount(const std::vector<SavedReplay>& replays, const std::string& filter);
	void DrawLibraryReplayRow(int index, SavedReplay& replay, const std::string& filter);
	void DrawRenameInput(int index, SavedReplay& replay);

	void DrawInGameReplays();
	void RefreshInGameCache();
	bool MatchesInGameFilter(const TheaterReplay& replay, const std::string& filter);
	int GetVisibleInGameCount(const std::string& filter);
	void DrawInGameReplayRow(int index, const TheaterReplay& replay, const std::string& filter);

	void DrawCurrentSession();
	void DrawActiveSessionTooltip();
	void UpdateCurrentSessionState(const std::string& path, std::string& outHash);
	void DrawSessionPath(const std::string& path);
	void DrawSessionActions(const std::string& path, const std::string& hash);

	void DrawDeleteLibraryReplay();
	void DrawDeleteGameReplay();

	void DrawSearchBar(const char* label, char* buffer, size_t bufferSize);

	char m_LibrarySearchBuffer[128] = "";
	int m_SelectedLibIndex = -1;

	char m_RenameBuffer[128] = "";
	int m_EditingIndex = -1;

	char m_InGameSearchBuffer[128] = "";
	std::vector<TheaterReplay> m_CachedInGameReplays;
	bool m_NeedsInGameRefresh = true;
	int m_SelectedGameIndex = -1;

	bool m_OpenLibDeleteModal = false;
	std::string m_HashToDelete = "";

	bool m_OpenGameDeleteModal = false;
	std::filesystem::path m_PathToDelete = "";
};