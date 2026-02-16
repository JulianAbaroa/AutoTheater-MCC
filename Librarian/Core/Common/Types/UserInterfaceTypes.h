#pragma once

#include "External/imgui/imgui.h"
#include <filesystem>
#include <vector>
#include <string>

// UI-specific metadata used for rendering on ImGui.
struct EventMetadata
{
	// Human-readable summary of the event shown in the UI.
	std::string Description;

	// List of internal engine string templates associated with this event category.
	// Used to map raw engine strings to this specific metadata entry.
	std::vector<std::wstring> InternalTemplates;
};

struct FilmMetadata
{
	std::string Author;
	std::string Info;
};

struct TheaterReplay
{
	FilmMetadata FilmMetadata{};
	std::filesystem::path FullPath{};
	std::string MovFileName{};
};

struct SavedReplay
{
	std::string Hash{};
	std::string DisplayName{};
	TheaterReplay TheaterReplay{};
	bool HasTimeline{};
};

struct PhaseUI
{
	const char* Name;
	ImVec4 Color;
};