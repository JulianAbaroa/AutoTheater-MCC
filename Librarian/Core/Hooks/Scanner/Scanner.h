#pragma once

#include "Core/Hooks/Scanner/Signatures.h"

namespace Scanner
{
	uintptr_t FindPattern(const Signature& sig);
	uintptr_t FindPattern(const char* pattern);
}