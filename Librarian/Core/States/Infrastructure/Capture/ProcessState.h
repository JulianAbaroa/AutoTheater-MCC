#pragma once

#include <atomic>

class ProcessState
{
public:
	uint32_t GetSessionID() const;
	void IncrementSessionID();

	HANDLE GetProcessHandle() const;
	void SetProcessHandle(HANDLE h);

	HANDLE GetLogReadHandle() const;
	void SetLogReadHandle(HANDLE h);

	HANDLE GetLogWriteHandle() const;
	void SetLogWriteHandle(HANDLE h);

	bool IsVideoConnected() const;
	void SetVideoConnected(bool connected);
	std::atomic<bool>* GetVideoConnectedFlag();

	bool IsAudioConnected() const;
	void SetAudioConnected(bool connected);
	std::atomic<bool>* GetAudioConnectedFlag();

	bool HasFatalError() const;
	void SetFatalError(bool hasError);

	void Cleanup();

private:
	uint32_t m_SessionID = 0;

	std::atomic<HANDLE> m_hProcess{ INVALID_HANDLE_VALUE };
	std::atomic<HANDLE> m_hLogRead{ INVALID_HANDLE_VALUE };
	std::atomic<HANDLE> m_hLogWrite{ INVALID_HANDLE_VALUE };

	std::atomic<bool> m_VideoConnected{ false };
	std::atomic<bool> m_AudioConnected{ false };

	std::atomic<bool> m_FFmpegReportedError{ false };
};