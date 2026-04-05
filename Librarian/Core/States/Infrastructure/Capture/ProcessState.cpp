#include "pch.h"
#include "Core/States/Infrastructure/Capture/ProcessState.h"

uint32_t ProcessState::GetSessionID() const { return m_SessionID; }
void ProcessState::IncrementSessionID() { m_SessionID++; }

HANDLE ProcessState::GetProcessHandle() const { return m_hProcess.load(); }
void ProcessState::SetProcessHandle(HANDLE h) { m_hProcess.store(h); }

HANDLE ProcessState::GetLogReadHandle() const { return m_hLogRead.load(); }
void ProcessState::SetLogReadHandle(HANDLE h) { m_hLogRead.store(h); }

HANDLE ProcessState::GetLogWriteHandle() const { return m_hLogWrite.load(); }
void ProcessState::SetLogWriteHandle(HANDLE h) { m_hLogWrite.store(h); }

bool ProcessState::IsVideoConnected() const { return m_VideoConnected.load(); }
void ProcessState::SetVideoConnected(bool connected) { m_VideoConnected.store(connected); }
std::atomic<bool>* ProcessState::GetVideoConnectedFlag() { return &m_VideoConnected; }

bool ProcessState::IsAudioConnected() const { return m_AudioConnected.load(); }
void ProcessState::SetAudioConnected(bool connected) { m_AudioConnected.store(connected); }
std::atomic<bool>* ProcessState::GetAudioConnectedFlag() { return &m_AudioConnected; }

bool ProcessState::HasFatalError() const { return m_FFmpegReportedError.load(); }
void ProcessState::SetFatalError(bool hasError) { m_FFmpegReportedError.store(hasError); }

void ProcessState::Cleanup()
{
	HANDLE hProcess = m_hProcess.exchange(INVALID_HANDLE_VALUE);
	if (hProcess != INVALID_HANDLE_VALUE && hProcess != NULL)
	{
		CloseHandle(hProcess);
	}

	HANDLE hLogRead = m_hLogRead.exchange(INVALID_HANDLE_VALUE);
	if (hLogRead != INVALID_HANDLE_VALUE && hLogRead != NULL)
	{
		CloseHandle(hLogRead);
	}

	HANDLE hLogWrite = m_hLogWrite.exchange(INVALID_HANDLE_VALUE);
	if (hLogWrite != INVALID_HANDLE_VALUE && hLogWrite != NULL)
	{
		CloseHandle(hLogWrite);
	}

	m_VideoConnected.store(false);
	m_AudioConnected.store(false);

	m_FFmpegReportedError.store(false);
}