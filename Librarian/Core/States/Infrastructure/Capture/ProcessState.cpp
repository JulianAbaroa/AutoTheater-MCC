#include "pch.h"
#include "Core/States/Infrastructure/Capture/ProcessState.h"

HANDLE ProcessState::GetProcessHandle() const { return m_hProcess.load(); }
void ProcessState::SetProcessHandle(HANDLE h) { m_hProcess.store(h); }

uint32_t ProcessState::GetSessionID() const { return m_SessionID; }
void ProcessState::IncrementSessionID() { m_SessionID++; }

std::atomic<bool>& ProcessState::GetVideoConnected() { return m_VideoConnected; }
std::atomic<bool>& ProcessState::GetAudioConnected() { return m_AudioConnected; }

void ProcessState::SetLogReadHandle(HANDLE handle) { m_hLogRead.store(handle); }

CaptureTelemetry& ProcessState::GetTelemetry()
{
	std::lock_guard<std::mutex> lock(m_TelemetryMutex);
	return m_Telemetry;
}

void ProcessState::UpdateLatency(bool isVideo, float latencyMs)
{
	std::lock_guard<std::mutex> lock(m_TelemetryMutex);

	if (isVideo) 
	{
		m_Telemetry.LastVideoWriteLatencyMs = latencyMs;

		if (latencyMs > m_Telemetry.MaxVideoWriteLatencyMs)
		{
			m_Telemetry.MaxVideoWriteLatencyMs = latencyMs;
		}
	}
	else {
		m_Telemetry.LastAudioWriteLatencyMs = latencyMs;

		if (latencyMs > m_Telemetry.MaxAudioWriteLatencyMs)
		{
			m_Telemetry.MaxAudioWriteLatencyMs = latencyMs;
		}
	}
}

void ProcessState::UpdateSpeed(float speed)
{
	std::lock_guard<std::mutex> lock(m_TelemetryMutex);

	m_Telemetry.FFmpegSpeed = speed;
}

void ProcessState::UpdateBitrate(std::string bitrateStr)
{
	std::lock_guard<std::mutex> lock(m_TelemetryMutex);

	m_Telemetry.CurrentBitrateKbps = std::stof(bitrateStr);
}

bool ProcessState::HasFatalError() const { return m_FFmpegReportedError.load(); }
void ProcessState::SetFatalError(bool value) { m_FFmpegReportedError.store(value); }

void ProcessState::Cleanup()
{
	HANDLE hProc = m_hProcess.exchange(INVALID_HANDLE_VALUE);
	if (hProc != INVALID_HANDLE_VALUE && hProc != NULL) 
	{
		CloseHandle(hProc);
	}

	HANDLE hLog = m_hLogRead.exchange(NULL);
	if (hLog != INVALID_HANDLE_VALUE && hLog != NULL) 
	{
		CloseHandle(hLog);
	}

	m_VideoConnected.store(false);
	m_AudioConnected.store(false);

	{
		std::lock_guard<std::mutex> lock(m_TelemetryMutex);
		m_Telemetry = {};
	}

	m_FFmpegReportedError.store(false);
}