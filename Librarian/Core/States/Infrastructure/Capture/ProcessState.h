#pragma once

#include "Core/Common/Types/FFmpegTypes.h"
#include "Windows.h"
#include <cstdint>
#include <atomic>
#include <mutex>

class ProcessState
{
public:
	HANDLE GetProcessHandle() const;
	void SetProcessHandle(HANDLE h);

	uint32_t GetSessionID() const;
	void IncrementSessionID();

	std::atomic<bool>& GetVideoConnected();
	std::atomic<bool>& GetAudioConnected();

	void SetLogReadHandle(HANDLE handle);

	// TODO: Possibly make a GetTelemetryCopy() for UI.
	CaptureTelemetry& GetTelemetry();
	void UpdateLatency(bool isVideo, float latencyMs);
	void UpdateSpeed(float speed);
	void UpdateBitrate(std::string bitrateStr);

	bool HasFatalError() const;
	void SetFatalError(bool value);

	void Cleanup();

private:
	std::atomic<HANDLE> m_hProcess{ INVALID_HANDLE_VALUE };
	std::atomic<HANDLE> m_hLogRead{ INVALID_HANDLE_VALUE };

	uint32_t m_SessionID = 0;

	std::atomic<bool> m_VideoConnected{ false };
	std::atomic<bool> m_AudioConnected{ false };

	CaptureTelemetry m_Telemetry;
	mutable std::mutex m_TelemetryMutex;

	std::atomic<bool> m_FFmpegReportedError{ false };
};