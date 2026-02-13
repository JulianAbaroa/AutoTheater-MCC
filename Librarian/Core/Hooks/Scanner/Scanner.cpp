#include "pch.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include <psapi.h>
#include <vector>

static uintptr_t Scan(uintptr_t base, size_t size, const char* pattern)
{
    auto patternToByte = [](const char* pattern) {
        auto bytes = std::vector<int>{};
        char* start = const_cast<char*>(pattern);
        char* end = const_cast<char*>(pattern) + strlen(pattern);

        for (char* current = start; current < end; ++current) {
            if (*current == ' ') continue;

            if (*current == '?') {
                current++;
                if (*current == '?') current++;
                bytes.push_back(-1);
            }
            else {
                bytes.push_back(strtoul(current, &current, 16));
            }
        }
        return bytes;
    };

    auto patternBytes = patternToByte(pattern);
    if (patternBytes.empty()) return 0;

    uint8_t* scanStart = reinterpret_cast<uint8_t*>(base);
    size_t patternSize = patternBytes.size();
    int* patternData = patternBytes.data();

    for (size_t i = 0; i < size - patternSize; ++i) {
        bool found = true;
        for (size_t j = 0; j < patternSize; ++j) {
            if (scanStart[i + j] != patternData[j] && patternData[j] != -1) {
                found = false;
                break;
            }
        }

        if (found) {
            return reinterpret_cast<uintptr_t>(&scanStart[i]);
        }
    }

    return 0;
}

uintptr_t Scanner::FindPattern(const Signature& sig)
{
	return FindPattern(sig.pattern);
}

uintptr_t Scanner::FindPattern(const char* pattern)
{
	HMODULE hModule = GetModuleHandle(L"haloreach.dll");
	if (!hModule) return 0;

	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(MODULEINFO));

	return Scan((uintptr_t)modInfo.lpBaseOfDll, modInfo.SizeOfImage, pattern);
}