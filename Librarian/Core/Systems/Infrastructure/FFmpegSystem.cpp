#include "pch.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Systems/CoreSystem.h"
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

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

float FFmpegSystem::GetRecordingDuration() const
{
	if (!g_pState->FFmpeg.IsRecording()) return 0.0f;

	auto now = std::chrono::steady_clock::now();
	auto duration = now - g_pState->FFmpeg.GetStartRecordingTime();

	return std::chrono::duration<float>(duration).count();
}

std::string FFmpegSystem::BuildFFmpegCommand(std::string outputPath, int width, int height, float fps, std::string videoPipeName, std::string audioPipeName)
{
	std::string ffmpegExe = g_pState->FFmpeg.GetExecutablePath();
	if (ffmpegExe.empty()) return "";

	std::string filename = this->GenerateTimestampName() + ".mkv";
	std::filesystem::path fullPath = std::filesystem::path(outputPath) / filename;

	std::string finalOutputFile = fullPath.string();

	int inW = width & ~1;
	int inH = height & ~1;
	int outW = g_pState->FFmpeg.GetTargetWidth();
	int outH = g_pState->FFmpeg.GetTargetHeight();

	std::string command;
	command.reserve(1024);
	char buffer[512]{};

	command += "\"";
	command += ffmpegExe;
	command += "\" -y -loglevel warning ";

	// Video
	snprintf(buffer, sizeof(buffer),
		"-f rawvideo -pix_fmt rgba -s %dx%d "
		"-r %.2f "
		"-probesize 32M -formatprobesize 0 -analyzeduration 0 -thread_queue_size 128 -i \"%s\" ",
		inW, inH, fps, videoPipeName.c_str());
	command += buffer;

	// Audio
	snprintf(buffer, sizeof(buffer),
		"-f f32le -ar 48000 -ac 8 -probesize 32M -formatprobesize 0 -analyzeduration 0 -thread_queue_size 128 -i \"%s\" ",
		audioPipeName.c_str());
	command += buffer;

	// Scale
	if (inW != outW || inH != outH)
	{
		snprintf(buffer, sizeof(buffer), "-vf \"scale=%d:%d:flags=bicubic\" ", outW, outH);
	}
	else
	{
		buffer[0] = '\0';
	}
	command += buffer;

	// Codecs & Output
	command += "-c:v h264_nvenc -preset p1 -tune hq -rc cbr -b:v 80M -maxrate 80M -bufsize 160M ";
	command += "-fps_mode cfr ";
	command += "-pix_fmt yuv420p -c:a aac -b:a 384k -map 0:v:0 -map 1:a:0 ";
	command += "-f matroska \"";
	command += finalOutputFile;
	command += "\"";

	return command;
}

bool FFmpegSystem::Start(const std::string& outputPath, int width, int height, float fps)
{
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
		DWORD err = GetLastError();

		if (!connected)
		{
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
			g_pUtil->Log.Append("[FFmpegSystem] INFO: Pipe connected: %s", name);
		}
		else
		{
			g_pUtil->Log.Append("[FFmpegSystem] ERROR: Failed to connect pipe: %s. Error Code: %lu", 
				name, GetLastError());
		}
	};

	std::thread(waitForConnection, g_pState->FFmpeg.GetVideoPipeHandle(), &m_VideoConnected, "Video").detach();
	std::thread(waitForConnection, g_pState->FFmpeg.GetAudioPipeHandle(), &m_AudioConnected, "Audio").detach();

	std::string cmd = this->BuildFFmpegCommand(outputPath, width, height, fps, videoPipeName, audioPipeName);
	if (!this->LaunchFFmpeg(cmd)) return false;

	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
	while (!m_VideoConnected.load())
	{
		if (std::chrono::steady_clock::now() > deadline)
		{
			g_pUtil->Log.Append("[FFmpegSystem] ERROR: Timeout waiting for video pipe.");
			this->ForceStop();
			return false;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	g_pState->FFmpeg.SetRecording(true);
	g_pUtil->Log.Append("[FFmpegSystem] INFO: Capture Session %d started.", m_SessionID.load());
	return true;
}

void FFmpegSystem::ForceStop()
{
	this->InternalStop(true);
}

void FFmpegSystem::Stop()
{
	this->InternalStop(false);
}


void FFmpegSystem::InitializeFFmpeg()
{
	std::string appData = g_pState->Settings.GetAppDataDirectory();
	if (appData.empty()) return;

	std::string exePath = appData + "\\ffmpeg.exe";
	g_pState->FFmpeg.SetExecutablePath(exePath);

	bool usable = this->VerifyExecutable(exePath);
	g_pState->FFmpeg.SetFFmpegInstalled(usable);

	if (usable) 
	{
		g_pUtil->Log.Append("[FFmpegSystem] INFO: Executable verified and usable.");
	}
	else 
	{
		if (GetFileAttributesA(exePath.c_str()) != INVALID_FILE_ATTRIBUTES) 
		{
			DeleteFileA(exePath.c_str());
			g_pUtil->Log.Append("[FFmpegSystem] WARNING: Incomplete executable found and removed.");
		}
	}

	if (g_pState->FFmpeg.GetOutputPath().empty())
	{
		std::filesystem::path defaultPath = g_pState->Settings.GetAppDataDirectory();
		defaultPath /= "Recordings";
		g_pState->FFmpeg.SetOutputPath(defaultPath.string());
	}
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


void FFmpegSystem::WriteVideo(void* data, size_t size)
{
	if (!g_pState->FFmpeg.IsRecording()) return;
	if (!this->WriteWithTimeout(g_pState->FFmpeg.GetVideoPipeHandle(), data, size, 50))
	{
		g_pUtil->Log.Append("[FFmpegSystem] ERROR: Aborting recording due to video block.");
		this->ForceStop();
	}
}

void FFmpegSystem::WriteAudio(const void* data, size_t size)
{
	if (!g_pState->FFmpeg.IsRecording()) return;
	if (!m_AudioConnected.load()) return;
	if (!this->WriteWithTimeout(g_pState->FFmpeg.GetAudioPipeHandle(), data, size, 50))
	{
		g_pUtil->Log.Append("[FFmpegSystem]ERROR: Aborting recording due to audio block.");
		this->ForceStop();
	}
}

bool FFmpegSystem::WriteWithTimeout(HANDLE hPipe, const void* data, size_t size, DWORD timeoutMs)
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
			if (GetLastError() == ERROR_IO_PENDING)
			{
				if (WaitForSingleObject(overlapped.hEvent, timeoutMs) == WAIT_OBJECT_0)
				{
					GetOverlappedResult(hPipe, &overlapped, &written, FALSE);
				}
				else
				{
					CancelIo(hPipe);
					finalSuccess = false;
					break;
				}
			}
			else
			{
				finalSuccess = false;
				break;
			}
		}

		if (written == 0) { finalSuccess = false; break; }
		ptr += written;
		remaining -= written;
	}

	CloseHandle(overlapped.hEvent);
	return finalSuccess;
}


void FFmpegSystem::Cleanup()
{
	g_pState->FFmpeg.Cleanup();
}


void FFmpegSystem::InternalStop(bool force)
{
	if (!g_pState->FFmpeg.IsRecording()) return;
	g_pState->FFmpeg.SetCaptureActive(false);
	g_pState->FFmpeg.SetRecording(false);

	HANDLE hVideo = g_pState->FFmpeg.GetVideoPipeHandle();
	HANDLE hAudio = g_pState->FFmpeg.GetAudioPipeHandle();

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
	g_pState->FFmpeg.SetVideoPipeHandle(INVALID_HANDLE_VALUE);
	ClosePipe(hAudio);
	g_pState->FFmpeg.SetAudioPipeHandle(INVALID_HANDLE_VALUE);

	HANDLE hProc = g_pState->FFmpeg.GetProcessHandle();
	if (hProc != INVALID_HANDLE_VALUE)
	{
		if (force)
		{
			TerminateProcess(hProc, 1);
			g_pUtil->Log.Append("[FFmpegSystem] WARNING: Process terminated (Force).");
		}
		else
		{
			if (WaitForSingleObject(hProc, 5000) == WAIT_TIMEOUT)
			{
				TerminateProcess(hProc, 0);
				g_pUtil->Log.Append("[FFmpegSystem] WARNING: Process timed out and killed.");
			}
			else
			{
				g_pUtil->Log.Append("[FFmpegSystem] INFO: Stop recording complete.");
			}
		}

		CloseHandle(hProc);
		g_pState->FFmpeg.SetProcessHandle(INVALID_HANDLE_VALUE);
	}

	g_pSystem->Video.StopRecording();
	g_pSystem->Audio.StopRecording();
	this->Cleanup();
}

bool FFmpegSystem::LaunchFFmpeg(const std::string& cmd)
{
	SECURITY_ATTRIBUTES sa;
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

	if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		CloseHandle(hRead);
		CloseHandle(hWrite);
		m_hLogRead.store(NULL);
		m_hLogWrite.store(NULL);
		return false;
	}

	g_pState->FFmpeg.SetProcessHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	CloseHandle(hWrite);
	m_hLogWrite.store(NULL);

	std::thread(&FFmpegSystem::ReadLogsThread, this, m_hLogRead.load()).detach();

	return true;
}

bool FFmpegSystem::CreatePipes(std::string& videoPipeName, std::string& audioPipeName)
{
	DWORD pid = GetCurrentProcessId();
	videoPipeName = "\\\\.\\pipe\\at_v_" + std::to_string(pid) + std::to_string(m_SessionID);
	audioPipeName = "\\\\.\\pipe\\at_a_" + std::to_string(pid) + std::to_string(m_SessionID);

	DWORD videoBufferSize = 256 * 1024 * 1024;
	DWORD audioBufferSize = 64 * 1024 * 1024;

	auto createPipe = [](const std::string& name, DWORD size) {
		HANDLE h = CreateNamedPipeA(
			name.c_str(),
			PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
			PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
			1, size, size, 0, NULL
		);

		if (h == INVALID_HANDLE_VALUE)
		{
			g_pUtil->Log.Append("[FFmpegSystem] ERROR: Failed to create pipe %s", name);
		}

		return h;
	};

	HANDLE hVideo = createPipe(videoPipeName, videoBufferSize);
	HANDLE hAudio = createPipe(audioPipeName, audioBufferSize);

	if (hVideo == INVALID_HANDLE_VALUE || hAudio == INVALID_HANDLE_VALUE)
		return false;

	g_pState->FFmpeg.SetVideoPipeHandle(hVideo);
	g_pState->FFmpeg.SetAudioPipeHandle(hAudio);
	return true;
}


void FFmpegSystem::ReadLogsThread(HANDLE hPipe) 
{
	if (hPipe == INVALID_HANDLE_VALUE || hPipe == NULL) return;

	char buffer[4096];
	DWORD bytesRead;
	std::string lineAccumulator;

	while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) 
	{
		buffer[bytesRead] = '\0';

		for (DWORD i = 0; i < bytesRead; i++) 
		{
			if (static_cast<unsigned char>(buffer[i]) < 32 &&
				buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != '\t') 
			{
				buffer[i] = '?';
			}
		}

		lineAccumulator += buffer;

		size_t pos;
		while ((pos = lineAccumulator.find('\n')) != std::string::npos) 
		{
			std::string currentLine = lineAccumulator.substr(0, pos);

			if (!currentLine.empty() && currentLine.back() == '\r') 
			{
				currentLine.pop_back();
			}

			if (!currentLine.empty()) 
			{
				g_pUtil->Log.Append("[FFmpeg] %s", currentLine.c_str());

				if (currentLine.find("Error") != std::string::npos ||
					currentLine.find("failed") != std::string::npos ||
					currentLine.find("Unknown") != std::string::npos ||
					currentLine.find("Invalid") != std::string::npos)
				{
					m_FFmpegReportedError.store(true);
				}
			}
			lineAccumulator.erase(0, pos + 1);
		}

		if (lineAccumulator.length() > 8192) 
		{
			g_pUtil->Log.Append("[FFmpeg] %s", lineAccumulator.c_str());
			lineAccumulator.clear();
		}
	}

	if (!lineAccumulator.empty()) 
	{
		g_pUtil->Log.Append("[FFmpeg] %s", lineAccumulator.c_str());
	}

	g_pUtil->Log.Append("[FFmpegSystem] INFO: Log thread closed.");
}

bool FFmpegSystem::IsAudioConnected() { return m_AudioConnected.load(); }

namespace
{
	class DownloadProgress : public IBindStatusCallback
	{
	public:
		STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
		{
			if (!g_pState->FFmpeg.IsDownloadInProgress())
			{
				return E_ABORT;
			}

			if (ulProgressMax > 0)
			{
				float percent = (static_cast<float>(ulProgress) / ulProgressMax) * 100.0f;
				g_pState->FFmpeg.SetDownloadProgress(percent);
			}

			return S_OK;
		}

		STDMETHOD(QueryInterface)(REFIID riid, void** ppv) 
		{
			if (riid == IID_IUnknown || riid == IID_IBindStatusCallback) 
			{
				*ppv = static_cast<IBindStatusCallback*>(this);
				return S_OK;
			}

			return E_NOINTERFACE;
		}

		STDMETHOD_(ULONG, AddRef)() { return 1; }
		STDMETHOD_(ULONG, Release)() { return 1; }
		STDMETHOD(OnStartBinding)(DWORD, IBinding*) { return S_OK; }
		STDMETHOD(GetPriority)(LONG*) { return S_OK; }
		STDMETHOD(OnLowResource)(DWORD) { return S_OK; }
		STDMETHOD(OnStopBinding)(HRESULT, LPCWSTR) { return S_OK; }
		STDMETHOD(GetBindInfo)(DWORD*, BINDINFO*) { return S_OK; }
		STDMETHOD(OnDataAvailable)(DWORD, DWORD, FORMATETC*, STGMEDIUM*) { return S_OK; }
		STDMETHOD(OnObjectAvailable)(REFIID, IUnknown*) { return S_OK; }
	};
}

bool FFmpegSystem::DownloadFFmpeg()
{
	if (!g_pState->Settings.ShouldUseAppData())
	{
		g_pUtil->Log.Append("[FFmpegSystem] WARNING: Download aborted. AppData usage is disabled.");
		return false;
	}

	if (g_pState->FFmpeg.IsDownloadInProgress()) return false;

	g_pState->FFmpeg.SetDownloadInProgress(true);
	g_pState->FFmpeg.SetDownloadProgress(0.0f);

	std::thread([this]() {
		g_pState->FFmpeg.SetDownloadInProgress(true);
		g_pUtil->Log.Append("[FFmpegSystem] INFO: Starting download from AutoTheater-Dependencies GitHub repository...");

		std::string url = "https://github.com/JulianAbaroa/AutoTheater-Dependencies/releases/download/FFmpeg/ffmpeg.exe";
		std::string target = g_pState->FFmpeg.GetExecutablePath();

		std::string dir = g_pState->Settings.GetAppDataDirectory();
		CreateDirectoryA(dir.c_str(), NULL);

		DownloadProgress progress;

		HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), target.c_str(), BINDF_GETNEWESTVERSION, &progress);

		if (SUCCEEDED(hr))
		{
			g_pState->FFmpeg.SetDownloadProgress(100.0f);
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			g_pUtil->Log.Append("[FFmpegSystem] INFO: Download successful.");
			g_pState->FFmpeg.SetFFmpegInstalled(true);
		}
		else
		{
			if (hr == E_ABORT)
			{
				g_pUtil->Log.Append("[FFmpegSystem] WARNING: Download canceled by user.");
			}
			else
			{
				g_pUtil->Log.Append("[FFmpegSystem] ERROR: Download failed. HRESULT: %s", hr);
			}

			DeleteFileA(target.c_str());
			g_pState->FFmpeg.SetFFmpegInstalled(false);
		}

		g_pState->FFmpeg.SetDownloadInProgress(false);
	}).detach();

	return true;
}

void FFmpegSystem::CancelDownload()
{
	g_pState->FFmpeg.SetDownloadInProgress(false);
	g_pUtil->Log.Append("[FFmpegSystem] WARNING: Download cancellation requested.");
}

bool FFmpegSystem::UninstallFFmpeg()
{
	std::string path = g_pState->FFmpeg.GetExecutablePath();

	DWORD dwAttrib = GetFileAttributesA(path.c_str());
	if (dwAttrib == INVALID_FILE_ATTRIBUTES) 
	{
		g_pUtil->Log.Append("[FFmpegSystem] ERROR: Uninstall failed, ffmpeg.exe not found.");
		return false;
	}

	if (DeleteFileA(path.c_str()))
	{
		g_pState->FFmpeg.SetFFmpegInstalled(false);
		g_pState->FFmpeg.SetDownloadInProgress(0.0f);
		g_pUtil->Log.Append("[FFmpegSystem] INFO: Uninstalled successfully.");
		return true;
	}
	else
	{
		g_pUtil->Log.Append("[FFmpegSystem] ERROR: Uninstall failed. Error code: %lu", GetLastError());
		return false;
	}
}