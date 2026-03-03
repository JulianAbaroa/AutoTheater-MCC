#pragma once

#include "Core/States/Infrastructure/ReplayState.h"

class ReplayManagerTab
{
public: 
	void Draw();

private:
	char LibrarySearchBuffer[128] = "";
	int SelectedLibIndex = -1;

	char RenameBuf[128] = "";
	int EditingIndex = -1;

	char InGameSearchBuffer[128] = "";
	std::vector<TheaterReplay> CachedInGameReplays;
	bool NeedsInGameRefresh = true;
	int SelectedGameIndex = -1;

	bool OpenLibDeleteModal = false;
	std::string HashToDelete = "";

	bool OpenGameDeleteModal = false;
	std::filesystem::path PathToDelete = "";

	void DrawLibrary();
	void DrawInGameReplays();
	void DrawCurrentSession();
	void DrawSearchBar(const char* label, char* buffer, size_t bufferSize);
};