#include "pch.h"
#include "Core/States/Infrastructure/Capture/PipeState.h"

HANDLE PipeState::GetVideoPipeHandle() const { return m_hVideoPipe.load(); }
void PipeState::SetVideoPipeHandle(HANDLE h) { m_hVideoPipe.store(h); }

HANDLE PipeState::GetAudioPipeHandle() const { return m_hAudioPipe.load(); }
void PipeState::SetAudioPipeHandle(HANDLE h) { m_hAudioPipe.store(h); }

int PipeState::GetConsecutiveWriteFailures() const { return m_ConsecutiveWriteFailures; }
void PipeState::IncrementConsecutiveWriteFailures() { m_ConsecutiveWriteFailures++; }
void PipeState::ResetConsecutiveWriteFailures() { m_ConsecutiveWriteFailures = 0; }
int PipeState::GetMaxConsecutiveWriteFailures() const { return m_MaxConsecutiveWriteFailures; }
double PipeState::GetMaxPipeDeadSeconds() const { return m_MaxPipeDeadSeconds; }

void PipeState::Cleanup()
{
	HANDLE hVideo = m_hVideoPipe.exchange(INVALID_HANDLE_VALUE);
	if (hVideo != INVALID_HANDLE_VALUE && hVideo != NULL)
	{
		CloseHandle(hVideo);
	}

	HANDLE hAudio = m_hAudioPipe.exchange(INVALID_HANDLE_VALUE);
	if (hAudio != INVALID_HANDLE_VALUE && hAudio != NULL)
	{
		CloseHandle(hAudio);
	}

	m_ConsecutiveWriteFailures = 0;
}