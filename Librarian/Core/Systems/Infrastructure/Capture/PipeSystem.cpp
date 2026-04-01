#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/PipeState.h"
#include "Core/States/Infrastructure/Capture/ProcessState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/PipeSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

bool PipeSystem::CreateVideoPipe(int width, int height)
{
	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	HANDLE hRead = NULL;
	HANDLE hWrite = NULL;

	DWORD frameSizeBytes = width * height * 4;
	DWORD videoBufferSize = frameSizeBytes * 10;

	g_pSystem->Debug->Log("[PipeSystem] INFO: Creating anonymous video"
		" pipe buffer: %lu MB.", videoBufferSize / (1024 * 1024));

	if (!CreatePipe(&hRead, &hWrite, &sa, videoBufferSize))
	{
		g_pSystem->Debug->Log("[PipeSystem] ERROR: Failed to create anonymous"
			" video pipe. WinError: %lu", GetLastError());

		return false;
	}

	SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, 0);

	g_pState->Infrastructure->Pipe->SetVideoReadHandle(hRead);
	g_pState->Infrastructure->Pipe->SetVideoWriteHandle(hWrite);

	return true;
}

bool PipeSystem::CreateAudioPipe()
{
	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	HANDLE hRead = NULL;
	HANDLE hWrite = NULL;

	DWORD audioBufferSize = 64 * 1024 * 1024;

	if (!CreatePipe(&hRead, &hWrite, &sa, audioBufferSize))
	{
		g_pSystem->Debug->Log("[PipeSystem] ERROR: Failed to create anonymous"
			" audio pipe. WinError: %lu", GetLastError());

		return false;
	}

	SetHandleInformation(hWrite, HANDLE_FLAG_INHERIT, 0);

	g_pState->Infrastructure->Pipe->SetAudioReadHandle(hRead);
	g_pState->Infrastructure->Pipe->SetAudioWriteHandle(hWrite);

	return true;
}


WriteResult PipeSystem::TryWriteVideo(void* data, size_t size)
{
	if (!g_pState->Infrastructure->FFmpeg->IsRecording() ||
		g_pState->Infrastructure->Process->HasFatalError())
		return WriteResult::FatalError;

	HANDLE hPipe = g_pState->Infrastructure->Pipe->GetVideoWriteHandle();
	if (hPipe == INVALID_HANDLE_VALUE) return WriteResult::FatalError;

	DWORD written = 0;

	if (!WriteFile(hPipe, data, (DWORD)size, &written, NULL))
	{
		DWORD err = GetLastError();
		g_pSystem->Debug->Log("[PipeSystem] ERROR: TryWriteVideo failed. WinErr=%lu", err);
		g_pState->Infrastructure->Process->SetFatalError(true);
		return WriteResult::FatalError;
	}

	m_LastVideoWriteTime = std::chrono::steady_clock::now();
	g_pState->Infrastructure->Pipe->ResetConsecutiveWriteFailures();
	return WriteResult::Success;
}

bool PipeSystem::WriteAudio(const void* data, size_t size)
{
	if (!g_pState->Infrastructure->FFmpeg->IsRecording() ||
		g_pState->Infrastructure->Process->HasFatalError())
	{
		return false;
	}

	HANDLE hPipe = g_pState->Infrastructure->Pipe->GetAudioWriteHandle();
	if (hPipe == INVALID_HANDLE_VALUE) return false;

	DWORD written = 0;

	if (!WriteFile(hPipe, data, (DWORD)size, &written, NULL))
	{
		g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Audio WriteFile failed."
			" WinErr=%lu", GetLastError());

		g_pState->Infrastructure->Process->SetFatalError(true);
		return false;
	}

	m_LastAudioWriteTime = std::chrono::steady_clock::now();
	return true;
}


void PipeSystem::Cleanup()
{
	m_LastVideoWriteTime = {};
	m_LastAudioWriteTime = {};

	g_pState->Infrastructure->Pipe->Cleanup();

	g_pSystem->Debug->Log("[PipeSystem] INFO: Cleanup completed.");
}