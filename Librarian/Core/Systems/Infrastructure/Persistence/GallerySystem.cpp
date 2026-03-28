#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/FFmpegState.h"
#include "Core/States/Infrastructure/Capture/DownloadState.h"
#include "Core/States/Infrastructure/Persistence/GalleryState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/RenderSystem.h"
#include "Core/Systems/Infrastructure/Persistence/GallerySystem.h"

GallerySystem::GallerySystem() 
{
	m_WorkerThread = std::thread(&GallerySystem::WorkerLoop, this);
	srand(static_cast<unsigned int>(time(NULL)));
}

GallerySystem::~GallerySystem() 
{
	m_ShuttingDown = true;
	m_Condition.notify_all();

	if (m_WorkerThread.joinable())
	{
		m_WorkerThread.join();
	}
}

void GallerySystem::WorkerLoop() 
{
	while (!m_ShuttingDown) 
	{
		int videoIndex = -1;

		{
			std::unique_lock<std::mutex> lock(m_QueueMutex);
			m_Condition.wait(lock, [this] {
				return !m_PendingMetadata.empty() || m_ShuttingDown;
				});

			if (m_ShuttingDown) break;

			videoIndex = m_PendingMetadata.front();
			m_PendingMetadata.pop();
		}

		if (videoIndex != -1) 
		{
			ProcessVideoMetadata(videoIndex);
		}
	}
}

void GallerySystem::LoadMetadataAsync(int videoIndex) 
{
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		m_PendingMetadata.push(videoIndex);
	}

	m_Condition.notify_one();
}

std::string GallerySystem::FormatDuration(float duration)
{
	if (duration <= 0.0f) return "Live / Recording...";

	int totalSeconds = static_cast<int>(duration);
	int hours = totalSeconds / 3600;
	int minutes = (totalSeconds % 3600) / 60;
	int seconds = totalSeconds % 60;

	if (hours > 0) 
	{
		return std::to_string(hours) + ":" +
			(minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" +
			(seconds < 10 ? "0" : "") + std::to_string(seconds);
	}
	else 
	{
		return (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" +
			(seconds < 10 ? "0" : "") + std::to_string(seconds);
	}
}

void GallerySystem::ProcessVideoMetadata(int videoIndex) 
{
	VideoData video = g_pState->Infrastructure->Gallery->GetVideo(videoIndex);
	if (video.FullPath.empty()) return;

	std::filesystem::path ffmpegPath = g_pState->Infrastructure->Download->GetExecutablePath();

	std::string durCmd = "\"" + ffmpegPath.string() + "\" -i \"" + video.FullPath.string() + "\" 2>&1";
	std::string infoOutput = ExecuteSilent(durCmd);

	float duration = 0.0f;
	size_t durPos = infoOutput.find("Duration: ");
	if (durPos != std::string::npos) 
	{
		int hh = 0, mm = 0;
		float ss = 0.0f;
		if (sscanf_s(infoOutput.c_str() + durPos + 10, "%d:%d:%f", &hh, &mm, &ss) == 3) 
		{
			duration = (hh * 3600.0f) + (mm * 60.0f) + ss;
		}
	}

	float seekTime = 1.0f;
	if (duration > 2.0f) 
	{
		float maxSeek = (duration > 10.0f) ? duration - 5.0f : duration - 1.0f;
		seekTime = 1.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (maxSeek - 1.0f)));
	}

	char timeBuf[64];
	sprintf_s(timeBuf, "%.3f", seekTime);

	std::string thumbCmd = "\"" + ffmpegPath.string() + "\" -v error -ss " + timeBuf +
		" -i \"" + video.FullPath.string() + "\" -vframes 1 -f image2 -vcodec bmp -an -";

	std::vector<unsigned char> imageData = ExecuteSilentBinary(thumbCmd);

	void* texturePtr = nullptr;
	if (!imageData.empty()) 
	{
		texturePtr = g_pSystem->Infrastructure->Render->CreateTextureFromMemory(imageData.data(), imageData.size());
	}

	g_pState->Infrastructure->Gallery->UpdateVideoMetadata(videoIndex, duration, texturePtr, video.FullPath.string());
}

void GallerySystem::ClearQueue() 
{
	std::lock_guard<std::mutex> lock(m_QueueMutex);
	std::queue<int> empty;
	std::swap(m_PendingMetadata, empty); 
}

void GallerySystem::RefreshList(const std::string& path)
{
	if (path.empty() || !std::filesystem::exists(path)) return;

	this->ClearQueue();

	g_pState->Infrastructure->Gallery->SetScanning(true);

	std::vector<VideoData> newBatch;
	const auto& oldVideos = g_pState->Infrastructure->Gallery->GetVideos();

	try {
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_regular_file() && 
				(entry.path().extension() == ".mkv" ||
				entry.path().extension() == ".mp4"))
			{
				VideoData data;
				data.FullPath = entry.path().string();
				data.FileName = entry.path().filename().string();
				data.FileSize = entry.file_size();

				auto it = std::find_if(oldVideos.begin(), oldVideos.end(), [&](const VideoData& v) {
					return v.FullPath == data.FullPath && v.FileSize == data.FileSize;
					});

				if (it != oldVideos.end()) 
				{
					data = *it;
				}

				newBatch.push_back(data);
			}
		}
	}
	catch (...) { }

	g_pState->Infrastructure->Gallery->SetVideos(std::move(newBatch));
	g_pState->Infrastructure->Gallery->SetScanning(false);
}

void GallerySystem::DeleteVideo(int videoIndex)
{
	VideoData video = g_pState->Infrastructure->Gallery->GetVideo(videoIndex);
	if (video.FullPath.empty()) return;

	g_pState->Infrastructure->Gallery->DeleteVideo(videoIndex);

	std::thread([path = video.FullPath]()
	{
		try
		{
			std::filesystem::path p(path);
			if (std::filesystem::exists(p))
				std::filesystem::remove(p);
		}
		catch (const std::filesystem::filesystem_error&) 
		{
			// ...
		}
	}).detach();
}

void GallerySystem::RenameVideo(int videoIndex, const std::string& newName)
{
	VideoData video = g_pState->Infrastructure->Gallery->GetVideo(videoIndex);
	if (video.FullPath.empty() || newName.empty()) return;

	std::string finalName = newName;

	if (std::filesystem::path(finalName).extension() != ".mkv") 
	{
		finalName += ".mkv";
	}

	if (finalName == video.FileName) return;

	std::filesystem::path oldPath(video.FullPath);
	std::filesystem::path newPath = oldPath.parent_path() / finalName;

	try 
	{
		if (std::filesystem::exists(newPath)) return;

		std::filesystem::rename(oldPath, newPath);

		g_pState->Infrastructure->Gallery->UpdateVideoName(videoIndex, finalName);
	}
	catch (const std::filesystem::filesystem_error&) 
	{ 
		return; 
	}
}


std::string GallerySystem::FormatBytes(uint64_t bytes)
{
	const char* units[] = { "B", "KB", "MB", "GB", "TB" };
	int i = 0;
	double dblBytes = static_cast<double>(bytes);

	while (dblBytes >= 1024 && i < 4)
	{
		dblBytes /= 1024;
		i++;
	}

	std::string s = std::to_string(dblBytes);
	size_t pos = s.find('.');
	if (pos != std::string::npos && pos + 3 < s.length()) {
		s = s.substr(0, pos + 3);
	}

	return s + " " + units[i];
}

size_t GallerySystem::GetPendingCount()
{
	std::lock_guard<std::mutex> lock(m_QueueMutex);
	return m_PendingMetadata.size();
}


std::string GallerySystem::ExecuteSilent(const std::string& command) 
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE hStdOutRead, hStdOutWrite;
	CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0);
	SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si = { sizeof(si) };
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdOutput = hStdOutWrite;
	si.hStdError = hStdOutWrite;
	si.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION pi = { 0 };
	if (CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) 
	{
		CloseHandle(hStdOutWrite);
		std::vector<char> buffer(8192);
		DWORD bytesRead;
		std::string output;

		while (ReadFile(hStdOutRead, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, NULL) && bytesRead > 0)
		{
			output.append(buffer.data(), bytesRead);
		}

		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CloseHandle(hStdOutRead);
		return output;
	}

	return "";
}

std::vector<unsigned char> GallerySystem::ExecuteSilentBinary(const std::string& command)
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE hStdOutRead, hStdOutWrite;
	if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0)) return {};

	SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOA si = { sizeof(si) };
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.hStdOutput = hStdOutWrite;
	si.hStdError = hStdOutWrite; 
	si.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION pi = { 0 };
	std::vector<unsigned char> output;

	if (CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
	{
		CloseHandle(hStdOutWrite);

		std::vector<unsigned char> readBuf(65536);
		DWORD bytesRead;

		while (ReadFile(hStdOutRead, readBuf.data(), static_cast<DWORD>(readBuf.size()), &bytesRead, NULL) && bytesRead > 0)
		{
			output.insert(output.end(), readBuf.begin(), readBuf.begin() + bytesRead);
		}

		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		CloseHandle(hStdOutRead);
	}
	else
	{
		CloseHandle(hStdOutWrite);
		CloseHandle(hStdOutRead);
	}

	return output;
}