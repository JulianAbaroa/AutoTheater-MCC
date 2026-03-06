#pragma once

#include <cstdint>
#include <atomic>

class TargetFPS
{
public:
	int GetCurrentFPSValue();

private:
	bool ResolveAddress();

	std::atomic<int32_t*> m_pTargetFPSAddr{ nullptr };
};