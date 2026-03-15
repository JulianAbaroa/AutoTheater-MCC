#pragma once

#include <chrono>

class ThreadSystem
{
public:
	bool WaitOrExit(std::chrono::milliseconds ms);
};