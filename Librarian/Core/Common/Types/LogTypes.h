#pragma once

#include <string>

enum class LogLevel
{
	Default,
	Info,
	Warning,
	Error
};

// Represents a single entry in the debug system, pre-formatted for ImGui rendering.
struct LogEntry
{
	std::string FullText;
	std::string Timestamp;
	std::string Tag;
	std::string MessagePrefix;
	std::string Message;
	LogLevel Level{};
};