#pragma once

#include "Core/Scanner/Signatures.h"

namespace Scanner
{
	uintptr_t FindPattern(const Signature& sig);
	uintptr_t FindPattern(const char* pattern);
}