#pragma once

#include <cstdint>

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