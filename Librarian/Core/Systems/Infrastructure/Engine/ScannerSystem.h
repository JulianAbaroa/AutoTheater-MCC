#pragma once

#include "Core/Common/Signatures.h"
#include <cstdint>

class ScannerSystem
{
public:
	uintptr_t FindPattern(const Signature& sig, const wchar_t* moduleName = L"haloreach.dll");
	uintptr_t FindPattern(const char* pattern, const wchar_t* moduleName = L"haloreach.dll");

private:
	uintptr_t Scan(uintptr_t base, size_t size, const char* pattern);
};