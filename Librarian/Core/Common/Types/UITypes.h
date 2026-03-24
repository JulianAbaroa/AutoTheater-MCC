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

// Metadata for replay files.
struct ReplayMetadata
{
	std::string Author;
	std::string Info;
	std::string Game{};
};

// Core replay data and file system references.
struct TheaterReplay
{
	ReplayMetadata ReplayMetadata{};
	std::filesystem::path FullPath{};
	std::string MovFileName{};
};

// Replay entry with unique identification and timeline status.
struct SavedReplay
{
	std::string Hash{};
	std::string DisplayName{};
	TheaterReplay TheaterReplay{};
	bool HasTimeline{};
};

// Visual definitions for ImGui elements.
struct PhaseUI
{
	const char* Name;
	ImVec4 Color;
};

struct LogFilterState
{
	std::string SearchStr;
	bool IsFiltering;
};