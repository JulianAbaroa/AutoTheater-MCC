#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/DownloadState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/AudioSystem.h"
#include "Core/Systems/Infrastructure/Capture/VideoSystem.h"
#include "Core/Systems/Infrastructure/Capture/FFmpegSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"
#include <filesystem>

bool FFmpegSystem::Start(const std::string& outputPath, int width, int height, float fps)
{
	g_pSystem->Debug->Log("[FFmpegSystem] Starting recording.");

	g_pSystem->Debug->Log("[FFmpegSystem] T0: Entering Start()");
	g_pSystem->Infrastructure->Video->PreallocatePool(width, height);
	g_pSystem->Debug->Log("[FFmpegSystem] T1: Pool ready, creating pipes");
	
	m_SessionID++;
	m_VideoConnected.store(false);
	m_AudioConnected.store(false);

	std::string videoPipeName, audioPipeName;
	if (!this->CreatePipes(videoPipeName, audioPipeName, width, height)) return false;
	g_pSystem->Debug->Log("[FFmpegSystem] T2: Pipes created, launching FFmpeg");

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

	std::thread(waitForConnection, g_pState->Infrastructure->FFmpeg->GetVideoPipeHandle(), &m_VideoConnected, "Video").detach();
	std::thread(waitForConnection, g_pState->Infrastructure->FFmpeg->GetAudioPipeHandle(), &m_AudioConnected, "Audio").detach();

	std::string cmd = this->BuildFFmpegCommand(outputPath, width, height, fps, videoPipeName, audioPipeName);
	if (!this->LaunchFFmpeg(cmd)) return false;
	g_pSystem->Debug->Log("[FFmpegSystem] T3: FFmpeg launched, waiting for pipe connection");

	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!m_VideoConnected.load())
	{
		if (std::chrono::steady_clock::now() > deadline)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Timeout waiting for video pipe.");
			this->ForceStop();
			return false;
		}

		HANDLE hProc = g_pState->Infrastructure->FFmpeg->GetProcessHandle();
		if (hProc != INVALID_HANDLE_VALUE)
		{
			DWORD exitCode;
			if (GetExitCodeProcess(hProc, &exitCode) && exitCode != STILL_ACTIVE)
			{
				g_pSystem->Debug->Log("[FFmpegSystem] ERROR: FFmpeg process died before connecting (exit code %lu).", exitCode);
				this->ForceStop();
				return false;
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	g_pSystem->Debug->Log("[FFmpegSystem] T4: Pipe connected, recording active");
	
	g_pState->Infrastructure->FFmpeg->SetRecording(true);
	g_pSystem->Debug->Log("[FFmpegSystem] INFO: Recording session %d started.", m_SessionID);
	return true;
}

void FFmpegSystem::ForceStop() { this->InternalStop(true); }
void FFmpegSystem::Stop() { this->InternalStop(false); }


void FFmpegSystem::InitializeDependencies()
{
	std::string appData = g_pState->Infrastructure->Settings->GetAppDataDirectory();
	bool shouldUseAppData = g_pState->Infrastructure->Settings->ShouldUseAppData();
	if (appData.empty() || !shouldUseAppData) return;

	std::string ffmpegDir = appData + "\\FFmpeg";
	CreateDirectoryA(ffmpegDir.c_str(), NULL);

	std::string exePath = ffmpegDir + "\\ffmpeg.exe";

	g_pState->Infrastructure->Download->SetExecutablePath(exePath);

	auto fileExists = [](const std::string& path) {
		return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
	};

	bool ffmpegOk = fileExists(exePath) && this->VerifyExecutable(exePath);
	g_pState->Infrastructure->Download->SetFFmpegInstalled(ffmpegOk);
	if (!ffmpegOk)
	{
		if (!ffmpegOk && fileExists(exePath))
		{
			DeleteFileA(exePath.c_str());
			g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Incomplete ffmpeg.exe found and removed.");
		}
	}

	// Set default videos output path.
	if (g_pState->Infrastructure->FFmpeg->GetOutputPath().empty())
	{
		std::filesystem::path defaultPath = appData;
		defaultPath /= "Recordings";
		g_pState->Infrastructure->FFmpeg->SetOutputPath(defaultPath.string());
	}
}


bool FFmpegSystem::WriteVideo(void* data, size_t size)
{
	if (!g_pState->Infrastructure->FFmpeg->IsRecording()) return false;
	if (m_FFmpegReportedError.load()) return false;

	HANDLE hPipe = g_pState->Infrastructure->FFmpeg->GetVideoPipeHandle();
	if (hPipe == INVALID_HANDLE_VALUE) return false;

	auto now = std::chrono::steady_clock::now();
	if (m_LastVideoWriteTime.time_since_epoch().count() != 0)
	{
		double secSinceLastWrite = std::chrono::duration<double>(now - m_LastVideoWriteTime).count();
		if (secSinceLastWrite >= k_PipeDeadSec)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] FATAL: Video pipe unresponsive for %.1fs", secSinceLastWrite);
			m_FFmpegReportedError.store(true);
			return false;
		}
	}

	if (this->WriteWithTimeout(hPipe, data, size, 5000, true))
	{
		m_LastVideoWriteTime = std::chrono::steady_clock::now();
		m_ConsecutiveWriteFailures = 0;
		return true;
	}

	m_ConsecutiveWriteFailures++;
	g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Video write failed (Timeout 5s). Failures: %d", m_ConsecutiveWriteFailures);

	if (m_ConsecutiveWriteFailures >= m_MaxConsecutiveWriteFailures)
	{
		g_pSystem->Debug->Log("[FFmpegSystem] FATAL: Giving up on video pipe.");
		m_FFmpegReportedError.store(true);
	}

	return false;
}

bool FFmpegSystem::WriteAudio(const void* data, size_t size)
{
	if (!g_pState->Infrastructure->FFmpeg->IsRecording()) return false;
	if (m_FFmpegReportedError.load()) return false;

	constexpr size_t frameSize = 8 * sizeof(float);
	size = (size / frameSize) * frameSize;
	if (size == 0) return true;

	HANDLE hPipe = g_pState->Infrastructure->FFmpeg->GetAudioPipeHandle();
	if (hPipe == INVALID_HANDLE_VALUE) return false;

	auto now = std::chrono::steady_clock::now();

	if (m_LastAudioWriteTime.time_since_epoch().count() != 0)
	{
		double secSinceLastWrite = std::chrono::duration<double>(now - m_LastAudioWriteTime).count();
		if (secSinceLastWrite >= k_PipeDeadSec)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Audio pipe unresponsive for %.1fs", secSinceLastWrite);
			m_FFmpegReportedError.store(true);
			return false;
		}
	}

	if (this->WriteWithTimeout(hPipe, data, size, 3000, false))
	{
		m_LastAudioWriteTime = now;
		return true;
	}

	g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Audio write TIMEOUT (2s), sample block dropped.");

	return true;
}


float FFmpegSystem::GetRecordingDuration() const
{
	if (!g_pState->Infrastructure->FFmpeg->IsRecording()) return 0.0f;

	auto now = std::chrono::steady_clock::now();

	auto startTime = g_pState->Infrastructure->FFmpeg->GetStartRecordingTime();

	std::chrono::duration<float> elapsed = now - startTime;

	return elapsed.count();
}

bool FFmpegSystem::HasFatalError() const { return m_FFmpegReportedError.load(); }


CaptureTelemetry FFmpegSystem::GetTelemetry() const
{
	std::lock_guard<std::mutex> lock(m_TelemetryMutex);
	return m_Telemetry;
}

void FFmpegSystem::UpdateQueueTelemetry()
{
	std::lock_guard<std::mutex> lock(m_TelemetryMutex);
	m_Telemetry.AudioPendingQueueSize = g_pThread->Capture->GetPendingAudioSize();
	m_Telemetry.VideoPendingQueueSize = g_pThread->Capture->GetPendingVideoSize();
}

void FFmpegSystem::Cleanup()
{
	m_FFmpegReportedError.store(false);
	m_hLogRead.store(NULL);
	m_hLogWrite.store(NULL);

	m_VideoConnected.store(false);
	m_AudioConnected.store(false);

	m_LastVideoWriteTime = {};
	m_LastAudioWriteTime = {};

	m_ConsecutiveWriteFailures = 0;

	g_pState->Infrastructure->FFmpeg->Cleanup();

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: Cleanup completed.");
}


bool FFmpegSystem::CreatePipes(std::string& videoPipeName, std::string& audioPipeName, int width, int height)
{
	DWORD pid = GetCurrentProcessId();
	videoPipeName = "\\\\.\\pipe\\at_v_" + std::to_string(pid) + std::to_string(m_SessionID);
	audioPipeName = "\\\\.\\pipe\\at_a_" + std::to_string(pid) + std::to_string(m_SessionID);

	DWORD frameSizesBytes = width * height * 4;
	auto encoderConfig = g_pState->Infrastructure->FFmpeg->GetEncoderConfig();
	DWORD videoBufferSize = frameSizesBytes * 16;
	DWORD audioBufferSize = 64 * 1024 * 1024;

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: Requesting video pipe buffer: %lu MB (%lu frames x %lu MB/frame)",
		videoBufferSize / (1024 * 1024),
		(DWORD)encoderConfig.MaxBufferedFrames,
		frameSizesBytes / (1024 * 1024));

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

		g_pSystem->Debug->Log("[FFmpegSystem] INFO: Pipe '%s' created OK. Requested buffer: %lu MB",
			name.c_str(), size / (1024 * 1024));

		return h;
	};

	HANDLE hVideo = createPipe(videoPipeName, videoBufferSize);
	HANDLE hAudio = createPipe(audioPipeName, audioBufferSize);

	if (hVideo == INVALID_HANDLE_VALUE || hAudio == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	g_pState->Infrastructure->FFmpeg->SetVideoPipeHandle(hVideo);
	g_pState->Infrastructure->FFmpeg->SetAudioPipeHandle(hAudio);
	return true;
}

std::string FFmpegSystem::BuildFFmpegCommand(std::string outputPath, int width, int height, float fps, std::string videoPipeName, std::string audioPipeName)
{
	std::string ffmpegExe = g_pState->Infrastructure->Download->GetExecutablePath();
	if (ffmpegExe.empty()) return "";

	auto encoderConfig = g_pState->Infrastructure->FFmpeg->GetEncoderConfig();

	std::string extension = (encoderConfig.OutputContainer == OutputContainer::MP4) ? ".mp4" : ".mkv";
	std::string formatTag = (encoderConfig.OutputContainer == OutputContainer::MP4) ? "mp4" : "matroska";

	std::string filename = this->GenerateTimestampName() + extension;
	std::filesystem::path fullPath = std::filesystem::path(outputPath) / filename;
	std::string finalOutputFile = fullPath.string();

	int inW = width & ~1;
	int inH = height & ~1;
	int outW = g_pState->Infrastructure->FFmpeg->GetTargetWidth() & ~1;
	int outH = g_pState->Infrastructure->FFmpeg->GetTargetHeight() & ~1;

	std::string v_codec;
	switch (encoderConfig.EncoderType)
	{
	case EncoderType::NVENC: v_codec = "h264_nvenc"; break;
	case EncoderType::AMF:   v_codec = "h264_amf";   break;
	case EncoderType::QSV:   v_codec = "h264_qsv";   break;
	default:                 v_codec = "libx264";
	}

	std::string cmd;
	cmd.reserve(2048);

	cmd += "\"" + ffmpegExe + "\" -y -loglevel info ";
	bool isScaling = (inW != outW || inH != outH);

	cmd += "-f rawvideo -pix_fmt rgba -s " + std::to_string(inW) + "x" + std::to_string(inH);
	cmd += " -r " + std::to_string(fps);
	cmd += " -thread_queue_size " + std::to_string(encoderConfig.MaxBufferedFrames);
	cmd += " -i \"" + videoPipeName + "\" ";

	int audioChannels = 2;
	auto instances = g_pState->Infrastructure->Audio->GetAllAudioInstances();
	for (const auto& [ptr, fmt] : instances)
	{
		if (fmt.Channels > audioChannels) audioChannels = fmt.Channels;
	}

	cmd += "-f f32le -ar 48000 -ac " + std::to_string(audioChannels);
	cmd += " -thread_queue_size " + std::to_string(encoderConfig.MaxAudioBufferedPackets);
	cmd += " -i \"" + audioPipeName + "\" ";
	const char* filterStrings[] = { "bicubic", "lanczos", "bilinear", "spline" };

	if (isScaling)
	{
		cmd += "-vf \"scale=" + std::to_string(outW) + ":" + std::to_string(outH);
		cmd += ":flags=" + std::string(filterStrings[(int)encoderConfig.ScalingFilter]) + ",format=yuv420p\" ";
	}
	else
	{
		cmd += "-vf \"format=yuv420p\" ";
	}

	cmd += "-c:v " + v_codec + " -profile:v main -pix_fmt yuv420p ";
	cmd += "-b:v " + std::to_string(encoderConfig.BitrateKbps) + "K ";
	cmd += "-maxrate " + std::to_string(encoderConfig.BitrateKbps) + "K ";
	cmd += "-bufsize " + std::to_string(encoderConfig.BitrateKbps * 2) + "K -fps_mode cfr ";

	if (encoderConfig.OutputContainer == OutputContainer::MP4)
	{
		cmd += "-movflags +frag_keyframe+empty_moov ";
	}
	else if (encoderConfig.OutputContainer == OutputContainer::MKV)
	{
		cmd += "-cluster_size_limit 2M -cluster_time_limit 1000 -reserve_index_space 1M ";
	}

	if (audioChannels == 8)
	{
		cmd += "-af \"pan=stereo|FL=FL+0.5*BL+0.5*SL|FR=FR+0.5*BR+0.5*SR\" ";
	}
	else if (audioChannels == 6)
	{
		cmd += "-af \"pan=stereo|FL=FL+0.3*FC+0.5*BL|FR=FR+0.3*FC+0.5*BR\" ";
	}

	cmd += "-c:a aac -b:a 384k -map 0:v:0 -map 1:a:0 -f " + formatTag + " \"" + finalOutputFile + "\"";

	return cmd;
}

std::string FFmpegSystem::GenerateTimestampName()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    std::tm parts;
    if (localtime_s(&parts, &now_c) != 0) return "0000-00-00_00-00-00";

    std::stringstream ss;
    ss << std::put_time(&parts, "%Y-%m-%d_%H-%M-%S");
    
    std::string result = ss.str();
    return result.empty() ? "timestamp_error" : result;
}

bool FFmpegSystem::LaunchFFmpeg(const std::string& cmd)
{
	SECURITY_ATTRIBUTES sa{ sizeof(sa), NULL, TRUE };
	HANDLE hLogRead = NULL, hLogWrite = NULL;

	if (!CreatePipe(&hLogRead, &hLogWrite, &sa, 0)) return false;

	SetHandleInformation(hLogRead, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si = { sizeof(si) };
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdError = hLogWrite;
	si.hStdOutput = hLogWrite;
	si.hStdInput = NULL;

	PROCESS_INFORMATION pi = { 0 };

	if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE,
		CREATE_NO_WINDOW | ABOVE_NORMAL_PRIORITY_CLASS,
		NULL, NULL, &si, &pi))
	{
		CloseHandle(hLogRead);
		CloseHandle(hLogWrite);
		return false;
	}

	struct { DWORD Version; DWORD ControlMask; DWORD StateMask; } powerThrottling = { 1, 0x4, 0 };
	SetProcessInformation(pi.hProcess, (PROCESS_INFORMATION_CLASS)0x4, &powerThrottling, sizeof(powerThrottling));

	g_pState->Infrastructure->FFmpeg->SetProcessHandle(pi.hProcess);
	m_hLogRead.store(hLogRead);

	CloseHandle(pi.hThread);
	CloseHandle(hLogWrite);

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: FFmpeg launched with PID %lu", pi.dwProcessId);

	std::thread(&FFmpegSystem::ReadLogsThread, this, hLogRead).detach();
	return true;
}


void FFmpegSystem::InternalStop(bool force)
{
	if (!g_pState->Infrastructure->FFmpeg->IsRecording()) return;

	g_pState->Infrastructure->FFmpeg->SetCaptureActive(false);
	g_pState->Infrastructure->FFmpeg->SetRecording(false);

	HANDLE hVideo = g_pState->Infrastructure->FFmpeg->GetVideoPipeHandle();
	HANDLE hAudio = g_pState->Infrastructure->FFmpeg->GetAudioPipeHandle();

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
	g_pState->Infrastructure->FFmpeg->SetVideoPipeHandle(INVALID_HANDLE_VALUE);

	ClosePipe(hAudio);
	g_pState->Infrastructure->FFmpeg->SetAudioPipeHandle(INVALID_HANDLE_VALUE);

	HANDLE hProc = g_pState->Infrastructure->FFmpeg->GetProcessHandle();
	if (hProc != INVALID_HANDLE_VALUE)
	{
		if (force)
		{
			TerminateProcess(hProc, 1);
			g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Process terminated (Force).");
		}
		else
		{
			if (WaitForSingleObject(hProc, 30000) == WAIT_TIMEOUT)
			{
				TerminateProcess(hProc, 0);
				g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Process timed out and killed.");
			}
			else
			{
				g_pSystem->Debug->Log("[FFmpegSystem] INFO: Stop recording complete.");
			}
		}

		CloseHandle(hProc);
		g_pState->Infrastructure->FFmpeg->SetProcessHandle(INVALID_HANDLE_VALUE);
	}

	this->Cleanup();
}

bool FFmpegSystem::VerifyExecutable(const std::string& path)
{
	DWORD dwAttrib = GetFileAttributesA(path.c_str());
	if (dwAttrib == INVALID_FILE_ATTRIBUTES || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		return false;
	}

	LARGE_INTEGER fileSize;
	HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		GetFileSizeEx(hFile, &fileSize);
		CloseHandle(hFile);
		if (fileSize.QuadPart < 10 * 1024 * 1024) return false;
	}

	std::string command = "\"" + path + "\" -version";
	STARTUPINFOA si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	if (CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) 
	{
		WaitForSingleObject(pi.hProcess, 1000);
		DWORD exitCode;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		return (exitCode == 0);
	}

	return false;
}


bool FFmpegSystem::WriteWithTimeout(HANDLE hPipe, const void* data, size_t size, DWORD timeoutMs, bool isVideo)
{
	if (hPipe == INVALID_HANDLE_VALUE) return false;

	OVERLAPPED overlapped = {};
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!overlapped.hEvent) return false;

	const BYTE* ptr = reinterpret_cast<const BYTE*>(data);
	size_t remaining = size;
	auto startTime = std::chrono::high_resolution_clock::now();
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

	auto endTime = std::chrono::high_resolution_clock::now();
	float latencyMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

	{
		std::lock_guard<std::mutex> lock(m_TelemetryMutex);
		if (isVideo) {
			m_Telemetry.LastVideoWriteLatencyMs = latencyMs;
			if (latencyMs > m_Telemetry.MaxVideoWriteLatencyMs) m_Telemetry.MaxVideoWriteLatencyMs = latencyMs;
		}
		else {
			m_Telemetry.LastAudioWriteLatencyMs = latencyMs;
			if (latencyMs > m_Telemetry.MaxAudioWriteLatencyMs) m_Telemetry.MaxAudioWriteLatencyMs = latencyMs;
		}
	}

	CloseHandle(overlapped.hEvent);
	return finalSuccess;
}


void FFmpegSystem::ReadLogsThread(HANDLE hPipe)
{
	if (hPipe == INVALID_HANDLE_VALUE || hPipe == NULL) return;

	std::vector<char> buffer(16384);
	DWORD bytesRead;
	std::string lineAccumulator;

	while (ReadFile(hPipe, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, NULL) && bytesRead > 0)
	{
		for (DWORD i = 0; i < bytesRead; i++) {
			unsigned char c = static_cast<unsigned char>(buffer[i]);
			if (c < 32 && c != '\n' && c != '\r' && c != '\t') {
				buffer[i] = '?';
			}
		}

		lineAccumulator.append(buffer.data(), bytesRead);

		size_t pos;
		while ((pos = lineAccumulator.find_first_of("\n\r")) != std::string::npos)
		{
			if (pos > 0) {
				std::string currentLine = lineAccumulator.substr(0, pos);
				TranslateAndLog(currentLine);
			}
			lineAccumulator.erase(0, pos + 1);

			while (!lineAccumulator.empty() && (lineAccumulator[0] == '\n' || lineAccumulator[0] == '\r')) {
				lineAccumulator.erase(0, 1);
			}
		}

		if (lineAccumulator.length() > static_cast<unsigned long long>(1024) * 64) lineAccumulator.clear();
	}

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: FFmpeg log thread closed.");
}

void FFmpegSystem::TranslateAndLog(const std::string& logLine)
{
	size_t speedPos = logLine.find("speed=");
	if (speedPos != std::string::npos)
	{
		try {
			std::string speedStr = logLine.substr(speedPos + 6);
			float s = std::stof(speedStr);
			std::lock_guard<std::mutex> lock(m_TelemetryMutex);
			m_Telemetry.FFmpegSpeed = s;
		}
		catch (...) { }
	}

	size_t bitratePos = logLine.find("bitrate=");
	if (bitratePos != std::string::npos)
	{
		try
		{
			std::string bitStr = logLine.substr(bitratePos + 8);
			size_t kbitsPos = bitStr.find("kbits/s");
			if (kbitsPos != std::string::npos)
			{
				bitStr = bitStr.substr(0, kbitsPos);

				std::lock_guard<std::mutex> lock(m_TelemetryMutex);
				m_Telemetry.CurrentBitrateKbps = std::stof(bitStr);
			}
		}
		catch (...) { }
	}

	if (speedPos == std::string::npos || bitratePos == std::string::npos)
	{
		g_pSystem->Debug->Log("[FFmpeg] %s", logLine.c_str());
	}
}