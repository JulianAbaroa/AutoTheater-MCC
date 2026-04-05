#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/PipeState.h"
#include "Core/States/Infrastructure/Capture/ProcessState.h"
#include "Core/States/Infrastructure/Capture/DownloadState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Capture/VideoState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Infrastructure/Capture/ProcessSystem.h"
#include "Core/Systems/Infrastructure/Capture/PipeSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"
#include <filesystem>

bool FFmpegSystem::Start(const std::string& outputPath, int width, int height, float fps)
{
	g_pSystem->Debug->Log("[FFmpegSystem] INFO: Starting recording.");

	g_pSystem->Infrastructure->Video->PreallocatePool(width, height);
	
	g_pState->Infrastructure->Process->IncrementSessionID();

	std::string videoPipeName;
	std::string audioPipeName;

	if (!g_pSystem->Infrastructure->Pipe->CreatePipes(
		videoPipeName, audioPipeName, width, height)) return false;

	auto waitForConnection = [this](HANDLE hPipe, std::atomic<bool>* flag, std::string name) {
		OVERLAPPED ov = {};
		ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!ov.hEvent) return;

		BOOL connected = ConnectNamedPipe(hPipe, &ov);

		if (!connected)
		{
			DWORD err = GetLastError();
			if (err == ERROR_IO_PENDING)
			{
				DWORD wait = WaitForSingleObject(ov.hEvent, 30000);
				if (wait == WAIT_OBJECT_0)
				{
					DWORD dummy;
					connected = GetOverlappedResult(hPipe, &ov, &dummy, FALSE);
				}
			}
			else if (err == ERROR_PIPE_CONNECTED)
			{
				connected = TRUE;
			}
		}

		CloseHandle(ov.hEvent);

		if (connected)
		{
			flag->store(true);
			g_pSystem->Debug->Log("[FFmpegSystem] INFO: Pipe connected: %s", name);
		}
		else
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Failed to connect pipe: %s. Error Code: %lu",
				name, GetLastError());
		}
	};

	std::thread(waitForConnection, 
		g_pState->Infrastructure->Pipe->GetVideoPipeHandle(), 
		g_pState->Infrastructure->Process->GetVideoConnectedFlag(), "Video").detach();

	std::thread(waitForConnection, 
		g_pState->Infrastructure->Pipe->GetAudioPipeHandle(), 
		g_pState->Infrastructure->Process->GetAudioConnectedFlag(), "Audio").detach();

	std::string cmd = g_pSystem->Infrastructure->Process->BuildFFmpegCommand(
		outputPath, width, height, fps, videoPipeName, audioPipeName);

	if (!g_pSystem->Infrastructure->Process->LaunchFFmpeg(cmd) ||
		!g_pSystem->Infrastructure->Pipe->WaitForVideoPipe()) return false;
	
	g_pState->Infrastructure->FFmpeg->SetRecording(true);

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: Recording session %d started.", 
		g_pState->Infrastructure->Process->GetSessionID());

	return true;
}

void FFmpegSystem::ForceStop() { this->InternalStop(true); }
void FFmpegSystem::Stop() { this->InternalStop(false); }


float FFmpegSystem::GetRecordingDuration() const
{
	if (!g_pState->Infrastructure->FFmpeg->IsRecording()) return 0.0f;

	auto now = std::chrono::steady_clock::now();

	auto startTime = g_pState->Infrastructure->FFmpeg->GetStartRecordingTime();

	std::chrono::duration<float> elapsed = now - startTime;

	return elapsed.count();
}

void FFmpegSystem::Cleanup()
{
	g_pState->Infrastructure->FFmpeg->Cleanup();
	g_pSystem->Infrastructure->Process->Cleanup();
	g_pSystem->Infrastructure->Pipe->Cleanup();

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: Cleanup completed.");
}


void FFmpegSystem::InternalStop(bool force)
{
	if (!g_pState->Infrastructure->FFmpeg->IsRecording()) return;

	g_pState->Infrastructure->FFmpeg->SetCaptureActive(false);
	g_pState->Infrastructure->FFmpeg->SetRecording(false);

	HANDLE hVideo = g_pState->Infrastructure->Pipe->GetVideoPipeHandle();
	HANDLE hAudio = g_pState->Infrastructure->Pipe->GetAudioPipeHandle();

	if (force && hVideo != INVALID_HANDLE_VALUE)
	{
		CancelIoEx(hVideo, NULL);
	}

	auto ClosePipe = [](HANDLE& h) {
		if (h != INVALID_HANDLE_VALUE)
		{
			CloseHandle(h);
			h = INVALID_HANDLE_VALUE;
		}
	};

	ClosePipe(hVideo);
	g_pState->Infrastructure->Pipe->SetVideoPipeHandle(INVALID_HANDLE_VALUE);

	ClosePipe(hAudio);
	g_pState->Infrastructure->Pipe->SetAudioPipeHandle(INVALID_HANDLE_VALUE);

	HANDLE hProcess = g_pState->Infrastructure->Process->GetProcessHandle();
	if (hProcess != INVALID_HANDLE_VALUE)
	{
		if (force)
		{
			TerminateProcess(hProcess, 1);
			g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Process terminated (Force).");
		}
		else
		{
			if (WaitForSingleObject(hProcess, 5000) == WAIT_TIMEOUT)
			{
				TerminateProcess(hProcess, 0);
				g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Process timed out and killed.");
			}
			else
			{
				g_pSystem->Debug->Log("[FFmpegSystem] INFO: Stop recording complete.");
			}
		}

		CloseHandle(hProcess);
		g_pState->Infrastructure->Process->SetProcessHandle(INVALID_HANDLE_VALUE);
	}

	this->Cleanup();
}