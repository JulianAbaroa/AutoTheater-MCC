#pragma once

#include <atomic>

class PipeState
{
public:
	HANDLE GetVideoPipeHandle() const;
	void SetVideoPipeHandle(HANDLE h);

	HANDLE GetAudioPipeHandle() const;
	void SetAudioPipeHandle(HANDLE h);

	int GetConsecutiveWriteFailures() const;
	void IncrementConsecutiveWriteFailures();
	void ResetConsecutiveWriteFailures();
	int GetMaxConsecutiveWriteFailures() const;
	double GetMaxPipeDeadSeconds() const;

	void Cleanup();

private:
	std::atomic<HANDLE> m_hVideoPipe{ INVALID_HANDLE_VALUE };
	std::atomic<HANDLE> m_hAudioPipe{ INVALID_HANDLE_VALUE };

	int m_ConsecutiveWriteFailures = 0;

	const int m_MaxConsecutiveWriteFailures = 5;
	static constexpr double m_MaxPipeDeadSeconds = 5.0;
};