#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/PipeState.h"
#include "Core/States/Infrastructure/Capture/ProcessState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/Capture/PipeSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"

using namespace std::chrono_literals;

bool PipeSystem::CreatePipes(std::string& videoPipeName, 
	std::string& audioPipeName, int width, int height)
{
	DWORD pid = GetCurrentProcessId();

	uint32_t sessionId = g_pState->Infrastructure->Process->GetSessionID();

	videoPipeName = "\\\\.\\pipe\\at_v_" + std::to_string(pid) + 
		std::to_string(sessionId);

	audioPipeName = "\\\\.\\pipe\\at_a_" + std::to_string(pid) + 
		std::to_string(sessionId);

	auto configuration = g_pState->Infrastructure->FFmpeg->GetConfiguration();

	DWORD frameSizesBytes = width * height * 4;
	DWORD videoBufferSize = frameSizesBytes * configuration.MaxBufferedFrames;
	DWORD audioBufferSize = 64 * 1024 * 1024;

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: Requesting video pipe buffer:"
		" %lu MB (%lu frames x %lu MB/frame)", videoBufferSize / (1024 * 1024),
		(DWORD)configuration.MaxBufferedFrames, frameSizesBytes / (1024 * 1024));

	auto createPipe = [](const std::string& name, DWORD size) {
		HANDLE h = CreateNamedPipeA(
			name.c_str(),
			PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
			1, size, size, 0, NULL);

		if (h == INVALID_HANDLE_VALUE)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Failed to create pipe %s. WinError: %lu",
				name.c_str(), GetLastError());
			return h;
		}

		return h;
	};

	HANDLE hVideo = createPipe(videoPipeName, videoBufferSize);
	HANDLE hAudio = createPipe(audioPipeName, audioBufferSize);

	if (hVideo == INVALID_HANDLE_VALUE || hAudio == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	g_pState->Infrastructure->Pipe->SetVideoPipeHandle(hVideo);
	g_pState->Infrastructure->Pipe->SetAudioPipeHandle(hAudio);

	return true;
}

bool PipeSystem::WaitForVideoPipe() const
{
	auto deadline = std::chrono::steady_clock::now() + 5000ms;
	while (!g_pState->Infrastructure->Process->IsVideoConnected())
	{
		if (std::chrono::steady_clock::now() > deadline)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Timeout waiting for video pipe.");
			g_pThread->Capture->StopRecording(true);
			return false;
		}

		HANDLE hProcess = g_pState->Infrastructure->Process->GetProcessHandle();
		if (hProcess != INVALID_HANDLE_VALUE)
		{
			DWORD exitCode;
			if (GetExitCodeProcess(hProcess, &exitCode) && exitCode != STILL_ACTIVE)
			{
				g_pSystem->Debug->Log("[FFmpegSystem] ERROR: FFmpeg process died"
					" before connecting (exit code %lu).", exitCode);
				g_pThread->Capture->StopRecording(true);
				return false;
			}
		}

		std::this_thread::sleep_for(5ms);
	}

	return true;
}


bool PipeSystem::WriteVideo(void* data, size_t size)
{
	bool isRecording = g_pState->Infrastructure->FFmpeg->IsRecording();
	bool hasFatalError = g_pState->Infrastructure->Process->HasFatalError();

	if (!isRecording || hasFatalError) return false;

	HANDLE hPipe = g_pState->Infrastructure->Pipe->GetVideoPipeHandle();
	if (hPipe == INVALID_HANDLE_VALUE) return false;

	auto now = std::chrono::steady_clock::now();

	double maxPipeDeadSeconds = g_pState->Infrastructure->Pipe->GetMaxPipeDeadSeconds();

	if (m_LastVideoWriteTime.time_since_epoch().count() != 0)
	{
		double secSinceLastWrite = std::chrono::duration<double>(now - m_LastVideoWriteTime).count();
		if (secSinceLastWrite >= maxPipeDeadSeconds)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Video pipe unresponsive for %.1fs", secSinceLastWrite);
			g_pState->Infrastructure->Process->SetFatalError(true);
			return false;
		}
	}

	if (this->WriteWithTimeout(hPipe, data, size, 5000, true))
	{
		m_LastVideoWriteTime = std::chrono::steady_clock::now();
		g_pState->Infrastructure->Pipe->ResetConsecutiveWriteFailures();
		return true;
	}

	g_pState->Infrastructure->Pipe->IncrementConsecutiveWriteFailures();
	g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Video write failed.");
	return false;
}

bool PipeSystem::WriteAudio(const void* data, size_t size)
{
	bool isRecording = g_pState->Infrastructure->FFmpeg->IsRecording();
	bool hasFatalError = g_pState->Infrastructure->Process->HasFatalError();

	if (!isRecording || hasFatalError) return false;

	constexpr size_t frameSize = 8 * sizeof(float);
	size = (size / frameSize) * frameSize;
	if (size == 0) return true;

	HANDLE hPipe = g_pState->Infrastructure->Pipe->GetAudioPipeHandle();
	if (hPipe == INVALID_HANDLE_VALUE) return false;

	auto now = std::chrono::steady_clock::now();

	double maxPipeDeadSeconds = g_pState->Infrastructure->Pipe->GetMaxPipeDeadSeconds();

	if (m_LastAudioWriteTime.time_since_epoch().count() != 0)
	{
		double secSinceLastWrite = std::chrono::duration<double>(now - m_LastAudioWriteTime).count();
		if (secSinceLastWrite >= maxPipeDeadSeconds)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Audio pipe unresponsive"
				" for %.1fs", secSinceLastWrite);

			g_pState->Infrastructure->Process->SetFatalError(true);
			return false;
		}
	}

	if (this->WriteWithTimeout(hPipe, data, size, 5000, false))
	{
		m_LastAudioWriteTime = now;
		return true;
	}

	g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Audio write timeout.");
	return false;
}

bool PipeSystem::WriteWithTimeout(HANDLE hPipe, const void* data, size_t size, DWORD timeoutMs, bool isVideo)
{
	if (hPipe == INVALID_HANDLE_VALUE) return false;

	OVERLAPPED overlapped = {};
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!overlapped.hEvent) return false;

	const BYTE* ptr = reinterpret_cast<const BYTE*>(data);
	size_t remaining = size;
	bool finalSuccess = true;

	while (remaining > 0)
	{
		DWORD toWrite = (DWORD)(std::min)(remaining, (size_t)0xFFFFFFFF);
		DWORD written = 0;

		if (!WriteFile(hPipe, ptr, toWrite, &written, &overlapped))
		{
			DWORD lastError = GetLastError();

			if (lastError == ERROR_PIPE_LISTENING || lastError == 536 || lastError == ERROR_NO_DATA)
			{
				finalSuccess = true;
				break;
			}
			else if (lastError == ERROR_IO_PENDING)
			{
				DWORD waitResult = WaitForSingleObject(overlapped.hEvent, timeoutMs);
				if (waitResult == WAIT_OBJECT_0)
				{
					if (!GetOverlappedResult(hPipe, &overlapped, &written, FALSE))
					{
						DWORD overErr = GetLastError();
						if (overErr == ERROR_PIPE_LISTENING || overErr == 536 || overErr == ERROR_NO_DATA || overErr == ERROR_BROKEN_PIPE)
						{
							finalSuccess = true;
							break;
						}

						g_pSystem->Debug->Log("[FFmpegSystem] ERROR: In GetOverlappedResult: %lu", overErr);
						finalSuccess = false;
						break;
					}
				}
				else if (waitResult == WAIT_TIMEOUT)
				{
					g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Timeout, the disk/FFmpeg did not accept data in %lu ms. (IsVideo: %d)", timeoutMs, isVideo);
					CancelIo(hPipe);
					finalSuccess = false;
					break;
				}
			}
			else
			{
				g_pSystem->Debug->Log("[FFmpegSystem] ERROR: WriteFile failed. Error: %lu (IsVideo: %d)", lastError, isVideo);
				finalSuccess = false;
				break;
			}
		}

		ptr += written;
		remaining -= written;
	}

	CloseHandle(overlapped.hEvent);
	return finalSuccess;
}


void PipeSystem::Cleanup()
{
	m_LastVideoWriteTime = {};
	m_LastAudioWriteTime = {};

	g_pState->Infrastructure->Pipe->Cleanup();

	g_pSystem->Debug->Log("[PipeSystem] INFO: Cleanup completed.");
}