#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/DownloadState.h"
#include "Core/States/Infrastructure/Capture/ProcessState.h"
#include "Core/States/Infrastructure/Capture/PipeState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/ProcessSystem.h"
#include "Core/Systems/Infrastructure/Engine/ThreadSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <filesystem>

void ProcessSystem::InitializeDependencies()
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

	bool isInstalled = fileExists(exePath) && this->VerifyExecutable(exePath);
	if (!isInstalled && fileExists(exePath))
	{
		DeleteFileA(exePath.c_str());
		g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Incomplete ffmpeg.exe"
			" found and removed.");
	}

	g_pState->Infrastructure->Download->SetFFmpegInstalled(isInstalled);

	// Set default videos output path.
	std::string outputPath = g_pState->Infrastructure->FFmpeg->GetOutputPath();
	if (outputPath.empty())
	{
		std::filesystem::path defaultPath = appData;
		defaultPath /= "Recordings";

		g_pState->Infrastructure->FFmpeg->SetOutputPath(defaultPath.string());
	}
}

bool ProcessSystem::VerifyExecutable(const std::string& path)
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
		if (fileSize.QuadPart < static_cast<long long>(10 * 1024) * 1024)
		{
			return false;
		}
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


std::string ProcessSystem::BuildFFmpegCommand(std::string outputPath, int width, int height, float fps)
{
	std::string ffmpegExe = g_pState->Infrastructure->Download->GetExecutablePath();
	if (ffmpegExe.empty()) return "";

	auto configuration = g_pState->Infrastructure->FFmpeg->GetConfiguration();

	std::string extension = (configuration.OutputContainer == OutputContainer::MP4) ? ".mp4" : ".mkv";
	std::string formatTag = (configuration.OutputContainer == OutputContainer::MP4) ? "mp4" : "matroska";
	std::string filename = this->GenerateTimestampName() + extension;

	std::filesystem::path fullPath = std::filesystem::path(outputPath) / filename;
	std::string finalOutputFile = fullPath.string();

	int inW = width & ~1;
	int inH = height & ~1;
	int outW = g_pState->Infrastructure->FFmpeg->GetTargetWidth() & ~1;
	int outH = g_pState->Infrastructure->FFmpeg->GetTargetHeight() & ~1;

	std::string v_codec;

	switch (configuration.EncoderType)
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
	cmd += " -thread_queue_size 8192 ";
	cmd += " -i pipe:0 ";

	int audioChannels = 2;
	auto instances = g_pState->Infrastructure->Audio->GetAllAudioInstances();

	for (const auto& [ptr, fmt] : instances)
	{
		if (fmt.Channels > audioChannels) audioChannels = fmt.Channels;
	}

	cmd += "-f f32le -ar 48000 -ac " + std::to_string(audioChannels);
	cmd += " -thread_queue_size 8192 ";
	cmd += " -i pipe:1 ";

	const char* filterStrings[] = { "bicubic", "lanczos", "bilinear", "spline" };

	if (isScaling)
	{
		cmd += "-vf \"scale=" + std::to_string(outW) + ":" + std::to_string(outH);
		cmd += ":flags=" + std::string(filterStrings[(int)configuration.ScalingFilter]) + ",format=yuv420p\" ";
	}
	else
	{
		cmd += "-vf \"format=yuv420p\" ";
	}

	cmd += "-c:v " + v_codec + " -profile:v main -pix_fmt yuv420p ";
	cmd += "-b:v " + std::to_string(configuration.BitrateKbps) + "K ";
	cmd += "-maxrate " + std::to_string(configuration.BitrateKbps) + "K ";
	cmd += "-bufsize " + std::to_string(configuration.BitrateKbps * 2) + "K -fps_mode vfr ";

	if (configuration.OutputContainer == OutputContainer::MP4)
	{
		cmd += "-movflags +frag_keyframe+empty_moov+default_base_moof ";
	}
	else if (configuration.OutputContainer == OutputContainer::MKV)
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

	cmd += "-c:a aac -b:a 384k ";
	cmd += "-max_interleave_delta 0 ";
	cmd += "-map 0:v:0 -map 1:a:0 -f " + formatTag + " \"" + finalOutputFile + "\"";

	return cmd;
}

bool ProcessSystem::LaunchFFmpeg(const std::string& cmd)
{
	SECURITY_ATTRIBUTES sa{ sizeof(sa), NULL, TRUE };
	HANDLE hLogRead = NULL;
	HANDLE hLogWrite = NULL;

	if (!CreatePipe(&hLogRead, &hLogWrite, &sa, 0)) return false;
	SetHandleInformation(hLogRead, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si = { sizeof(si) };
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdError = hLogWrite;

	si.hStdOutput = g_pState->Infrastructure->Pipe->GetAudioReadHandle();
	si.hStdInput = g_pState->Infrastructure->Pipe->GetVideoReadHandle();

	PROCESS_INFORMATION pi = { 0 };

	if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, TRUE,
		CREATE_NO_WINDOW | ABOVE_NORMAL_PRIORITY_CLASS,
		NULL, NULL, &si, &pi))
	{
		CloseHandle(hLogRead);
		CloseHandle(hLogWrite);
		return false;
	}

	HANDLE hVidRead = g_pState->Infrastructure->Pipe->GetVideoReadHandle();
	if (hVidRead != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hVidRead);
		g_pState->Infrastructure->Pipe->SetVideoReadHandle(INVALID_HANDLE_VALUE);
	}

	HANDLE hAudRead = g_pState->Infrastructure->Pipe->GetAudioReadHandle();
	if (hAudRead != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hAudRead);
		g_pState->Infrastructure->Pipe->SetAudioReadHandle(INVALID_HANDLE_VALUE);
	}

	struct { DWORD Version; DWORD ControlMask; DWORD StateMask; } powerThrottling = { 1, 0x4, 0 };
	SetProcessInformation(pi.hProcess, (PROCESS_INFORMATION_CLASS)0x4, &powerThrottling, sizeof(powerThrottling));

	g_pState->Infrastructure->Process->SetProcessHandle(pi.hProcess);
	g_pState->Infrastructure->Process->SetLogReadHandle(hLogRead);

	CloseHandle(pi.hThread);
	CloseHandle(hLogWrite);

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: FFmpeg launched with PID %lu", pi.dwProcessId);

	std::thread([this, hLogRead]() {
		this->ReadLogsThread(hLogRead);
	}).detach();

	return true;
}


void ProcessSystem::ReadLogsThread(HANDLE hPipe)
{
	if (hPipe == INVALID_HANDLE_VALUE || hPipe == NULL) return;

	std::vector<char> buffer(16384);
	DWORD bytesRead;
	std::string lineAccumulator;

	while (ReadFile(hPipe, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, NULL) && bytesRead > 0)
	{
		for (DWORD i = 0; i < bytesRead; i++) 
		{
			unsigned char c = static_cast<unsigned char>(buffer[i]);

			if (c < 32 && c != '\n' && c != '\r' && c != '\t') 
			{
				buffer[i] = '?';
			}
		}

		lineAccumulator.append(buffer.data(), bytesRead);

		size_t pos;
		while ((pos = lineAccumulator.find_first_of("\n\r")) != std::string::npos)
		{
			if (pos > 0) 
			{
				std::string currentLine = lineAccumulator.substr(0, pos);
				TranslateAndLog(currentLine);
			}

			lineAccumulator.erase(0, pos + 1);

			while (!lineAccumulator.empty() && 
				(lineAccumulator[0] == '\n' || 
					lineAccumulator[0] == '\r')) 
			{
				lineAccumulator.erase(0, 1);
			}
		}

		if (lineAccumulator.length() > static_cast<unsigned long long>(1024) * 64) lineAccumulator.clear();
	}

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: FFmpeg log pipe closed.");

	HANDLE hProc = g_pState->Infrastructure->Process->GetProcessHandle();
	if (hProc != INVALID_HANDLE_VALUE)
	{
		DWORD exitCode = 0;
		if (GetExitCodeProcess(hProc, &exitCode))
		{
			g_pSystem->Debug->Log("[FFmpegSystem] INFO: FFmpeg exit code: %lu", exitCode);
		}
	}

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: FFmpeg log thread closed.");
}

void ProcessSystem::TranslateAndLog(const std::string& logLine)
{
	size_t speedPos = logLine.find("speed=");
	if (speedPos != std::string::npos)
	{
		try 
		{
			std::string speedStr = logLine.substr(speedPos + 6);
			float speed = std::stof(speedStr);
			
			g_pState->Infrastructure->Process->UpdateSpeed(speed);
		}
		catch (...) {}
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

				g_pState->Infrastructure->Process->UpdateBitrate(bitStr);
			}
		}
		catch (...) {}
	}

	g_pSystem->Debug->Log("[FFmpeg] %s", logLine.c_str());
}


void ProcessSystem::Cleanup()
{
	g_pState->Infrastructure->Process->Cleanup();

	g_pSystem->Debug->Log("[ProcessSystem] INFO: Cleanup completed.");
}


std::string ProcessSystem::GenerateTimestampName()
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