#pragma once

#include <cstdint>
#include <atomic>

class TargetFramerateHook
{
public:
	int GetCurrentFramerateValue();
	void Cleanup();

private:
	bool ResolveAddress();

	std::atomic<int32_t*> m_pTargetFramerateAddr{ nullptr };
};