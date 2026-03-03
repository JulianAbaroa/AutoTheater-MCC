#pragma once

#include <mutex>

class LogUtil
{
public:
	void Append(const char* format, ...);

private:
	std::mutex m_Mutex;
};