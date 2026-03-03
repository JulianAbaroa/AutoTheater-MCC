#pragma once

#include "Core/Common/Types/LogTypes.h"
#include <string>
#include <mutex>

class DebugSystem
{
public:
	void AddLog(LogEntry entry);
	void RemoveLogsIf(std::function<bool(const LogEntry&)> predicate);
};