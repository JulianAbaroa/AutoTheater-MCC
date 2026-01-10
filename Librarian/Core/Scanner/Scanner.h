#pragma once

#include "Core/Scanner/Signatures.h"
#include "windows.h"
#include <cstdint>

namespace Scanner
{
	uintptr_t FindPattern(const Signature& sig);
	uintptr_t FindPattern(const char* pattern);
}