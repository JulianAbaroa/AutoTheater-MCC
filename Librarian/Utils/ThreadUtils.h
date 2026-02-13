#pragma once

#include <chrono>

namespace ThreadUtils
{
	bool WaitOrExit(std::chrono::milliseconds ms);
}