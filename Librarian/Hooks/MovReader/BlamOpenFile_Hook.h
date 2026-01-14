#pragma once

#include <cstdint>

extern std::string g_FilmPath;

typedef void(__fastcall* BlamOpenFile_t)(
	long long fileContext,
	uint32_t accessFlags,
	uint32_t* translatedStatus
);

namespace BlamOpenFile_Hook
{
	void Install();
	void Uninstall();
}