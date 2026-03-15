#pragma once

#include "Core/Common/Types/MuxerTypes.h"
#include <Windows.h>
#include <atomic>

class MuxerState
{
public:
	void SetReady(bool v);
	bool IsReady() const;

	void SetStartTime(double t);
	double GetStartTime() const;

private:
	std::atomic<bool> m_Ready{ false };
	std::atomic<double> m_StartTime{ 0.0 };
};