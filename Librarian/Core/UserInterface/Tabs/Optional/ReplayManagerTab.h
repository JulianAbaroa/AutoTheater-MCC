#pragma once

#include "Core/States/Infrastructure/ReplayState.h"

class ReplayManagerTab
{
public: 
	void Draw();

private:
	void DrawLibrary();
	void DrawInGameReplays();
	void DrawCurrentSession();
	void DrawSearchBar(char* buffer, size_t bufferSize);

	char SearchBuffer[128] = "";

	int SelectedLibIndex = -1;
	int EditingIndex = -1;
	char RenameBuf[128] = "";

	std::vector<TheaterReplay> CachedInGameReplays;
	bool NeedsInGameRefresh = true;
	int SelectedGameIndex = -1;

	bool OpenLibDeleteModal = false;
	std::string HashToDelete = "";

	bool OpenGameDeleteModal = false;
	std::filesystem::path PathToDelete = "";
};