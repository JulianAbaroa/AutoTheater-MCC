#pragma once

#include "Windows.h"
#include <atomic>

class PipeState
{
public:
	HANDLE GetVideoReadHandle() const;
	void SetVideoReadHandle(HANDLE h);

	HANDLE GetVideoWriteHandle() const;
	void SetVideoWriteHandle(HANDLE h);

	HANDLE GetAudioReadHandle() const;
	void SetAudioReadHandle(HANDLE h);

	HANDLE GetAudioWriteHandle() const;
	void SetAudioWriteHandle(HANDLE h);

	int GetConsecutiveWriteFailures() const;
	void IncrementConsecutiveWriteFailures();
	void ResetConsecutiveWriteFailures();
	int GetMaxConsecutiveWriteFailures();
	double GetMaxPipeDeadSeconds();

	void Cleanup();

private:
	std::atomic<HANDLE> m_hVideoRead{ INVALID_HANDLE_VALUE };
	std::atomic<HANDLE> m_hVideoWrite{ INVALID_HANDLE_VALUE };

	std::atomic<HANDLE> m_hAudioRead{ INVALID_HANDLE_VALUE };
	std::atomic<HANDLE> m_hAudioWrite{ INVALID_HANDLE_VALUE };

	std::atomic<int> m_ConsecutiveWriteFailures = 0;

	const int m_MaxConsecutiveWriteFailures = 5;
	static constexpr double m_MaxPipeDeadSeconds = 5.0;
};