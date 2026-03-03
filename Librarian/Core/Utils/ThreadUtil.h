#pragma once

#include <chrono>

class ThreadUtil
{
public:
	bool WaitOrExit(std::chrono::milliseconds ms);
};