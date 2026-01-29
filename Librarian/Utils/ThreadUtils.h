#pragma once

#include "Core/Common/GlobalState.h"
#include <chrono>

namespace ThreadUtils
{
	bool WaitOrExit(std::chrono::milliseconds ms);
}