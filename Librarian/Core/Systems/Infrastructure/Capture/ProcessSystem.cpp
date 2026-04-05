#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/AudioState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/ProcessState.h"
#include "Core/States/Infrastructure/Capture/DownloadState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Capture/ProcessSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <filesystem>

void ProcessSystem::InitializeDependencies()
{
	std::string appDataDir = g_pState->Infrastructure->Settings->GetAppDataDirectory();
	bool shouldUseAppData = g_pState->Infrastructure->Settings->ShouldUseAppData();
	if (appDataDir.empty() || !shouldUseAppData) return;

	std::string ffmpegDir = appDataDir + "\\FFmpeg";
	CreateDirectoryA(ffmpegDir.c_str(), NULL);

	std::string exePath = ffmpegDir + "\\ffmpeg.exe";
	g_pState->Infrastructure->Download->SetExecutablePath(exePath);

	auto fileExists = [](const std::string& path) {
		return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
	};

	bool isValid = fileExists(exePath) && this->VerifyExecutable(exePath);
	if (!isValid)
	{
		DeleteFileA(exePath.c_str());

		g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Incomplete ffmpeg.exe"
			" found and removed.");
	}

	g_pState->Infrastructure->Download->SetFFmpegInstalled(isValid);

	// Set default videos output path.
	if (g_pState->Infrastructure->FFmpeg->GetOutputPath().empty())
	{
		std::filesystem::path defaultPath = 
			std::filesystem::path(appDataDir) / "Recordings";

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


std::string ProcessSystem::BuildFFmpegCommand(
	std::string outputPath, int width, int height, float framerate, 
	std::string videoPipeName, std::string audioPipeName)
{
	std::string ffmpegExe = g_pState->Infrastructure->Download->GetExecutablePath();
	if (ffmpegExe.empty()) return "";

	auto configuration = g_pState->Infrastructure->FFmpeg->GetConfiguration();

	std::string extension = 
		(configuration.OutputContainer == OutputContainer::MP4) ? ".mp4" : ".mkv";

	std::string cmd;
	cmd.reserve(2048);

	cmd += "\"" + ffmpegExe + "\" -y -loglevel info ";

	cmd += "-probesize 32M -analyzeduration 32M ";

	int outWidth = g_pState->Infrastructure->FFmpeg->GetTargetWidth();
	int outHeight = g_pState->Infrastructure->FFmpeg->GetTargetHeight();
	bool isScaling = (width != outWidth || height != outHeight);

	cmd += "-f rawvideo -pix_fmt rgba -s " + std::to_string(width) + "x" + std::to_string(height);
	cmd += " -r " + std::to_string(framerate);
	cmd += " -thread_queue_size " + std::to_string(configuration.MaxBufferedFrames);
	cmd += " -i \"" + videoPipeName + "\" ";

	int audioChannels = 2;
	auto instances = g_pState->Infrastructure->Audio->GetAllAudioInstances();
	for (const auto& [ptr, fmt] : instances)
	{
		if (fmt.Channels > audioChannels) audioChannels = fmt.Channels;
	}

	cmd += "-f f32le -ar 48000 -ac " + std::to_string(audioChannels);
	cmd += " -thread_queue_size " + std::to_string(configuration.MaxAudioBufferedPackets);
	cmd += " -i \"" + audioPipeName + "\" ";
	const char* filterStrings[] = { "bicubic", "lanczos", "bilinear", "spline" };

	if (isScaling)
	{
		cmd += "-vf \"scale=" + std::to_string(outWidth) + ":" + std::to_string(outHeight);
		cmd += ":flags=" + std::string(filterStrings[(int)configuration.ScalingFilter]) + ",format=yuv420p\" ";
	}
	else
	{
		cmd += "-vf \"format=yuv420p\" ";
	}

	std::string v_codec;
	switch (configuration.EncoderType)
	{
	case EncoderType::NVENC: v_codec = "h264_nvenc"; break;
	case EncoderType::AMF:   v_codec = "h264_amf";   break;
	case EncoderType::QSV:   v_codec = "h264_qsv";   break;
	default:                 v_codec = "libx264";
	}

	cmd += "-c:v " + v_codec + " -profile:v main -pix_fmt yuv420p ";
	cmd += "-b:v " + std::to_string(configuration.BitrateKbps) + "K ";
	cmd += "-maxrate " + std::to_string(configuration.BitrateKbps) + "K ";
	cmd += "-bufsize " + std::to_string(configuration.BitrateKbps * 2) + "K -fps_mode cfr ";

	if (configuration.OutputContainer == OutputContainer::MP4)
	{
		cmd += "-movflags +frag_keyframe+empty_moov ";
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

	std::string formatTag =
		(configuration.OutputContainer == OutputContainer::MP4) ? "mp4" : "matroska";

	std::string filename = this->GenerateTimestampName() + extension;
	std::filesystem::path fullPath = std::filesystem::path(outputPath) / filename;
	std::string finalOutputFile = fullPath.string();

	cmd += "-c:a aac -b:a 384k -map 0:v:0 -map 1:a:0 -f " + formatTag + " \"" + finalOutputFile + "\"";

	return cmd;
}

bool ProcessSystem::LaunchFFmpeg(const std::string& cmd)
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

	struct { DWORD Version; DWORD ControlMask; DWORD StateMask; } powerThrottling = 
		{ 1, 0x4, 0 };

	SetProcessInformation(pi.hProcess, (PROCESS_INFORMATION_CLASS)0x4, 
		&powerThrottling, sizeof(powerThrottling));

	g_pState->Infrastructure->Process->SetProcessHandle(pi.hProcess);
	g_pState->Infrastructure->Process->SetLogReadHandle(hLogRead);
	g_pState->Infrastructure->Process->SetLogWriteHandle(hLogWrite);

	CloseHandle(pi.hThread);
	CloseHandle(hLogWrite);

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: FFmpeg launched with PID %lu", 
		pi.dwProcessId);

	std::thread(&ProcessSystem::ReadLogsThread, this, hLogRead).detach();
	return true;
}


void ProcessSystem::ReadLogsThread(HANDLE hPipe)
{
	if (hPipe == INVALID_HANDLE_VALUE || hPipe == NULL) return;

	std::vector<char> buffer(16384);
	DWORD bytesRead;
	std::string lineAccumulator;

	while (ReadFile(hPipe, buffer.data(), static_cast<DWORD>(buffer.size()), 
		&bytesRead, NULL) && bytesRead > 0)
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
				this->TranslateAndLog(currentLine);
			}

			lineAccumulator.erase(0, pos + 1);

			while (!lineAccumulator.empty() && 
				(lineAccumulator[0] == '\n' || lineAccumulator[0] == '\r')) 
			{
				lineAccumulator.erase(0, 1);
			}
		}

		if (lineAccumulator.length() > static_cast<unsigned long long>(1024) * 64)
		{
			lineAccumulator.clear();
		}
	}

	g_pSystem->Debug->Log("[FFmpegSystem] INFO: FFmpeg log thread closed.");
}

void ProcessSystem::TranslateAndLog(const std::string& logLine)
{
	//size_t speedPos = logLine.find("speed=");
	//if (speedPos != std::string::npos)
	//{
	//	try {
	//		std::string speedStr = logLine.substr(speedPos + 6);
	//		float s = std::stof(speedStr);
	//	}
	//	catch (...) {}
	//}

	//size_t bitratePos = logLine.find("bitrate=");
	//if (bitratePos != std::string::npos)
	//{
	//	try
	//	{
	//		std::string bitStr = logLine.substr(bitratePos + 8);
	//		size_t kbitsPos = bitStr.find("kbits/s");
	//		if (kbitsPos != std::string::npos)
	//		{
	//			bitStr = bitStr.substr(0, kbitsPos);
	//		}
	//	}
	//	catch (...) {}
	//}

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