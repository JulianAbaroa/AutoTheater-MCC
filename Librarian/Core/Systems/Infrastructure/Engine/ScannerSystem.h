#pragma once

#include "Core/Common/Signatures.h"
#include <cstdint>

class ScannerSystem
{
public:
	uintptr_t FindPattern(const Signature& sig);
	uintptr_t FindPattern(const char* pattern);

private:
	uintptr_t Scan(uintptr_t base, size_t size, const char* pattern);
};