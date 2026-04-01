#include "pch.h"
#include "Core/States/Infrastructure/Capture/PipeState.h"

HANDLE PipeState::GetVideoReadHandle() const { return m_hVideoRead.load(); }
void PipeState::SetVideoReadHandle(HANDLE h) { m_hVideoRead.store(h); }

HANDLE PipeState::GetVideoWriteHandle() const { return m_hVideoWrite.load(); }
void PipeState::SetVideoWriteHandle(HANDLE h) { m_hVideoWrite.store(h); }

HANDLE PipeState::GetAudioReadHandle() const { return m_hAudioRead.load(); }
void PipeState::SetAudioReadHandle(HANDLE h) { m_hAudioRead.store(h); }

HANDLE PipeState::GetAudioWriteHandle() const { return m_hAudioWrite.load(); }
void PipeState::SetAudioWriteHandle(HANDLE h) { m_hAudioWrite.store(h); }

int PipeState::GetConsecutiveWriteFailures() const { return m_ConsecutiveWriteFailures.load(); }
void PipeState::IncrementConsecutiveWriteFailures() { m_ConsecutiveWriteFailures++; }
void PipeState::ResetConsecutiveWriteFailures() { m_ConsecutiveWriteFailures.store(0); }
int PipeState::GetMaxConsecutiveWriteFailures() { return m_MaxConsecutiveWriteFailures; }
double PipeState::GetMaxPipeDeadSeconds() { return m_MaxPipeDeadSeconds; }

void PipeState::Cleanup()
{
	auto closeHandleSafe = [](std::atomic<HANDLE>& handleAtomic) {
		HANDLE h = handleAtomic.exchange(INVALID_HANDLE_VALUE);
		if (h != INVALID_HANDLE_VALUE && h != NULL) 
		{
			CloseHandle(h);
		}
	};

	closeHandleSafe(m_hVideoRead);
	closeHandleSafe(m_hVideoWrite);
	closeHandleSafe(m_hAudioRead);
	closeHandleSafe(m_hAudioWrite);

	m_ConsecutiveWriteFailures.store(0);
}