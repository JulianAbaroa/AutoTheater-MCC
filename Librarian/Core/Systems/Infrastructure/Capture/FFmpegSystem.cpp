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
#include <filesystem>

bool FFmpegSystem::Start(const std::string& outputPath, int width, int height, float fps)
{
	g_pSystem->Infrastructure->Video->PreallocatePool(width, height);

	m_SessionID++;
	m_VideoConnected.store(false);
	m_AudioConnected.store(false);

	std::string videoPipeName, audioPipeName;
	if (!this->CreatePipes(videoPipeName, audioPipeName)) return false;

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

	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
	while (!m_VideoConnected.load())
	{
		if (std::chrono::steady_clock::now() > deadline)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Timeout waiting for video pipe.");
			this->ForceStop();
			return false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	
	auto now = std::chrono::steady_clock::now();
	g_pState->Infrastructure->FFmpeg->SetStartRecordingTime(now);
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

	if (!m_AudioConnected.load())
	{
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - g_pState->Infrastructure->FFmpeg->GetStartRecordingTime()).count();
		if (elapsed < 2000)
		{
			bool ok = this->WriteWithTimeout(
				g_pState->Infrastructure->FFmpeg->GetVideoPipeHandle(), data, size, 50);
			if (ok) m_LastVideoWriteTime = std::chrono::steady_clock::now();
			return true;
		}
	}

	auto now = std::chrono::steady_clock::now();

	if (m_LastVideoWriteTime.time_since_epoch().count() != 0)
	{
		double secSinceLastWrite = std::chrono::duration<double>(
			now - m_LastVideoWriteTime).count();

		if (secSinceLastWrite >= k_PipeDeadSec)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Video pipe unresponsive for %.1fs,"
				" signaling fatal error.", secSinceLastWrite);
			m_FFmpegReportedError.store(true);
			return false;
		}
	}

	int retryCount = 0;
	while (retryCount < 10)
	{
		if (this->WriteWithTimeout(
			g_pState->Infrastructure->FFmpeg->GetVideoPipeHandle(), data, size, 50))
		{
			m_LastVideoWriteTime = std::chrono::steady_clock::now();
			m_ConsecutiveWriteFailures = 0;
			return true;
		}
		retryCount++;
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Video write failed after 10 retries");
	m_ConsecutiveWriteFailures++;

	if (m_ConsecutiveWriteFailures >= m_MaxConsecutiveWriteFailures)
	{
		g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Pipe appears dead (%d consecutive failures),"
			" signaling fatal error.", m_ConsecutiveWriteFailures);
		m_FFmpegReportedError.store(true);
	}

	return false;
}

bool FFmpegSystem::WriteAudio(const void* data, size_t size)
{
	if (!g_pState->Infrastructure->FFmpeg->IsRecording()) return false;

	constexpr size_t frameSize = 8 * sizeof(float);
	size = (size / frameSize) * frameSize;
	if (size == 0) return true;

	auto now = std::chrono::steady_clock::now();

	if (m_LastAudioWriteTime.time_since_epoch().count() != 0)
	{
		double secSinceLastWrite = std::chrono::duration<double>(
			now - m_LastAudioWriteTime).count();

		if (secSinceLastWrite >= k_PipeDeadSec)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Audio pipe unresponsive for %.1fs,"
				" signaling fatal error.", secSinceLastWrite);
			m_FFmpegReportedError.store(true);
			return false;
		}
	}

	m_LastAudioWriteTime = now;

	int retryCount = 0;
	while (retryCount < 5)
	{
		if (this->WriteWithTimeout(
			g_pState->Infrastructure->FFmpeg->GetAudioPipeHandle(), data, size, 50))
		{
			return true;
		}
		retryCount++;
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Audio write failed, continuing");
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


bool FFmpegSystem::CreatePipes(std::string& videoPipeName, std::string& audioPipeName)
{
	DWORD pid = GetCurrentProcessId();

	videoPipeName = "\\\\.\\pipe\\at_v_" + std::to_string(pid) + std::to_string(m_SessionID);
	audioPipeName = "\\\\.\\pipe\\at_a_" + std::to_string(pid) + std::to_string(m_SessionID);

	auto encoderConfig = g_pState->Infrastructure->FFmpeg->GetEncoderConfig();

	DWORD videoBufferSize = encoderConfig.VideoBufferPipeSize * 1024 * 1024;
	DWORD audioBufferSize = 64 * 1024 * 1024;

	auto createPipe = [](const std::string& name, DWORD size) {
		HANDLE h = CreateNamedPipeA(
			name.c_str(),
			PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
			1, size, size, 0, NULL);

		if (h == INVALID_HANDLE_VALUE)
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Failed to create pipe %s", name);
		}

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
	int outW = g_pState->Infrastructure->FFmpeg->GetTargetWidth();
	int outH = g_pState->Infrastructure->FFmpeg->GetTargetHeight();

	std::string v_codec;
	switch (encoderConfig.EncoderType)
	{
	case EncoderType::NVENC: v_codec = "h264_nvenc"; break;
	case EncoderType::AMF:   v_codec = "h264_amf";   break;
	case EncoderType::QSV:   v_codec = "h264_qsv";   break;
	case EncoderType::CPU:   v_codec = "libx264";    break;
	default:                 v_codec = "libx264";
	}

	const char* filterStrings[] = { "bicubic", "lanczos", "bilinear", "spline" };

	std::string command;
	command.reserve(1024);
	char buffer[1024]{};

	command += "\"";
	command += ffmpegExe;
	command += "\" -y -loglevel info ";

	int audioChannels = 2;
	auto instances = g_pState->Infrastructure->Audio->GetAllAudioInstances();
	for (const auto& [ptr, fmt] : instances)
	{
		if (fmt.Channels > audioChannels)
			audioChannels = fmt.Channels;
	}

	// Video & Audio
	const char* probeLimit = (inW > 1920) ? "64M" : "32M";
	snprintf(buffer, sizeof(buffer),
		"-f rawvideo -pix_fmt rgba -s %dx%d -r %.2f -probesize %s -formatprobesize 0 -analyzeduration 0 -thread_queue_size %d -i \"%s\" "
		"-f f32le -ar 48000 -ac %d -probesize 32M -formatprobesize 0 -analyzeduration 0 -thread_queue_size %d -i \"%s\" ",
		inW, inH, fps, probeLimit, encoderConfig.ThreadQueueSize, videoPipeName.c_str(),
		audioChannels, encoderConfig.ThreadQueueSize, audioPipeName.c_str());
	command += buffer;

	// Scale
	if (inW != outW || inH != outH)
	{
		snprintf(buffer, sizeof(buffer), "-vf \"scale=%d:%d:flags=%s,format=yuv420p\" ",
			outW, outH, filterStrings[(int)encoderConfig.ScalingFilter]);
		command += buffer;
	}
	else
	{
		command += "-vf \"format=yuv420p\" ";
	}

	snprintf(buffer, sizeof(buffer),
		"-c:v %s -b:v %dK -maxrate %dK -bufsize %dK -fps_mode cfr ",
		v_codec.c_str(), encoderConfig.BitrateKbps,
		encoderConfig.BitrateKbps, encoderConfig.BitrateKbps * 2);
	command += buffer;

	if (encoderConfig.OutputContainer == OutputContainer::MP4)
	{
		command += "-movflags +faststart ";
	}

	std::string audioFilter;
	if (audioChannels == 8)
	{
		audioFilter = "-af \"pan=stereo|FL=FL+0.5*BL+0.5*SL|FR=FR+0.5*BR+0.5*SR\" ";
	}
	else if (audioChannels == 6)
	{
		audioFilter = "-af \"pan=stereo|FL=FL+0.3*FC+0.5*BL|FR=FR+0.3*FC+0.5*BR\" ";
	}
	else audioFilter = "";

	command += audioFilter;

	command += "-c:a aac -b:a 384k -map 0:v:0 -map 1:a:0 ";

	snprintf(buffer, sizeof(buffer), "-f %s \"%s\"", formatTag.c_str(), finalOutputFile.c_str());
	command += buffer;

	return command;
}

std::string FFmpegSystem::GenerateTimestampName()
{
	auto now = std::chrono::system_clock::now();
	std::time_t now_c = std::chrono::system_clock::to_time_t(now);

	std::tm parts;
	if (localtime_s(&parts, &now_c) != 0) return "0000-00-00_00-00-00";

	char buffer[64]{};
	if (strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &parts) > 0)
	{
		return std::string(buffer);
	}

	return "timestamp_error";
}

bool FFmpegSystem::LaunchFFmpeg(const std::string& cmd)
{
	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	HANDLE hRead = NULL;
	HANDLE hWrite = NULL;

	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;

	m_hLogRead.store(hRead);
	m_hLogWrite.store(hWrite);

	SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si = { sizeof(STARTUPINFOA) };
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.hStdInput = NULL;

	PROCESS_INFORMATION pi = { 0 };

	if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE,
		CREATE_NO_WINDOW | HIGH_PRIORITY_CLASS | CREATE_SUSPENDED,
		NULL, NULL, &si, &pi))
	{
		CloseHandle(hRead);
		CloseHandle(hWrite);
		m_hLogRead.store(NULL);
		m_hLogWrite.store(NULL);
		return false;
	}

	SetPriorityClass(pi.hProcess, HIGH_PRIORITY_CLASS);

	struct {
		DWORD Version;
		DWORD ControlMask;
		DWORD StateMask;
	} powerThrottling = { 0 };

	powerThrottling.Version = 1;
	powerThrottling.ControlMask = 0x4;
	powerThrottling.StateMask = 0;

	SetProcessInformation(pi.hProcess, (PROCESS_INFORMATION_CLASS)0x4,
		&powerThrottling, sizeof(powerThrottling));

	SetThreadPriority(pi.hThread, THREAD_PRIORITY_HIGHEST);

	DWORD_PTR processAffinity, systemAffinity;
	if (GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity))
	{
		SetProcessAffinityMask(pi.hProcess, processAffinity);
	}

	ResumeThread(pi.hThread);

	g_pState->Infrastructure->FFmpeg->SetProcessHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hWrite);
	m_hLogWrite.store(NULL);

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: FFmpeg launched.");

	std::thread(&FFmpegSystem::ReadLogsThread, this, m_hLogRead.load()).detach();
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
			if (WaitForSingleObject(hProc, 5000) == WAIT_TIMEOUT)
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

	g_pSystem->Infrastructure->Video->StopRecording();
	g_pSystem->Infrastructure->Audio->StopRecording();
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


bool FFmpegSystem::WriteWithTimeout(HANDLE hPipe, const void* data, size_t size, DWORD timeoutMs)
{
	if (hPipe == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	OVERLAPPED overlapped = {};
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!overlapped.hEvent) return false;

	const BYTE* ptr = reinterpret_cast<const BYTE*>(data);
	size_t remaining = size;
	bool finalSuccess = true;
	DWORD lastError = 0;

	while (remaining > 0)
	{
		DWORD toWrite = (DWORD)(std::min)(remaining, (size_t)0xFFFFFFFF);
		DWORD written = 0;

		if (!WriteFile(hPipe, ptr, toWrite, &written, &overlapped))
		{
			lastError = GetLastError();

			if (lastError == ERROR_IO_PENDING)
			{
				DWORD waitResult = WaitForSingleObject(overlapped.hEvent, timeoutMs);

				if (waitResult == WAIT_OBJECT_0)
				{
					if (!GetOverlappedResult(hPipe, &overlapped, &written, FALSE))
					{
						finalSuccess = false;
						break;
					}
				}
				else if (waitResult == WAIT_TIMEOUT)
				{
					CancelIo(hPipe);
					CloseHandle(overlapped.hEvent);
					return false;
				}
				else
				{
					finalSuccess = false;
					break;
				}
			}
			else if (lastError == ERROR_NO_DATA || lastError == ERROR_PIPE_NOT_CONNECTED)
			{
				CloseHandle(overlapped.hEvent);
				return false;
			}
			else
			{
				g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Write error %lu, continuing", lastError);
				finalSuccess = false;
				break;
			}
		}

		if (written == 0)
		{
			finalSuccess = false;
			break;
		}

		ptr += written;
		remaining -= written;
	}

	CloseHandle(overlapped.hEvent);
	return finalSuccess;
}


void FFmpegSystem::ReadLogsThread(HANDLE hPipe)
{
	if (hPipe == INVALID_HANDLE_VALUE || hPipe == NULL) return;

	char buffer[4096]{};
	DWORD bytesRead;
	std::string lineAccumulator;

	while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
	{
		buffer[bytesRead] = '\0';

		for (DWORD i = 0; i < bytesRead; i++) {
			if (static_cast<unsigned char>(buffer[i]) < 32 &&
				buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != '\t') {
				buffer[i] = '?';
			}
		}

		lineAccumulator += buffer;

		size_t pos;
		while ((pos = lineAccumulator.find_first_of("\n\r")) != std::string::npos)
		{
			std::string currentLine = lineAccumulator.substr(0, pos);

			if (!currentLine.empty())
			{
				TranslateAndLog(currentLine);
			}

			lineAccumulator.erase(0, pos + 1);
		}

		if (lineAccumulator.length() > 8192) lineAccumulator.clear();
	}

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: FFmpeg log thread closed.");
}

void FFmpegSystem::TranslateAndLog(const std::string& logLine)
{
	bool isProgressLine = (logLine.find("frame=") != std::string::npos);

	size_t speedPos = logLine.find("speed=");
	if (speedPos != std::string::npos)
	{
		try {
			std::string speedStr = logLine.substr(speedPos + 6);
			float s = std::stof(speedStr);
			g_pState->Infrastructure->FFmpeg->SetRecordingSpeed(s);
		}
		catch (...) {  }
	}

	if (!isProgressLine)
	{
		g_pSystem->Debug->Log("[FFmpeg] %s", logLine.c_str());
	}

	static const std::string_view criticalErrors[] = {
		"Could not open encoder", "Task finished with error code",
		"failed to open", "Error while opening encoder"
	};

	for (const auto& err : criticalErrors)
	{
		if (logLine.find(err) != std::string::npos)
		{
			m_FFmpegReportedError.store(true);
			g_pSystem->Debug->Log("[FFmpeg] ERROR: %s", logLine.c_str());
			break;
		}
	}
}