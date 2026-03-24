#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Capture/DownloadState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/Capture/DownloadSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <filesystem>
#include <urlmon.h>
#include <shlobj.h>
#include <string>
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "urlmon.lib")

class DownloadSystem::DownloadProgress : public IBindStatusCallback
{
public:
	STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
	{
		if (!g_pState->Infrastructure->Download->IsDownloadInProgress())
		{
			return E_ABORT;
		}

		if (ulProgressMax > 0)
		{
			float percent = (static_cast<float>(ulProgress) / ulProgressMax) * 100.0f;
			g_pState->Infrastructure->Download->SetDownloadProgress(percent);
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


bool DownloadSystem::DownloadDependencies()
{
	if (!g_pState->Infrastructure->Settings->ShouldUseAppData()) return false;
	if (g_pState->Infrastructure->Download->IsDownloadInProgress()) return false;

	g_pState->Infrastructure->Download->SetDownloadInProgress(true);
	g_pState->Infrastructure->Download->SetDownloadProgress(0.0f);

	std::thread([this]() {
		std::string appData = g_pState->Infrastructure->Settings->GetAppDataDirectory();
		std::string ffmpegDir = appData + "\\FFmpeg";
		CreateDirectoryA(ffmpegDir.c_str(), NULL);

		std::string exePath = ffmpegDir + "\\ffmpeg.exe";

		DownloadProgress progress;

		g_pSystem->Debug->Log("[FFmpegSystem] INFO: Downloading ffmpeg.exe.");
		HRESULT hr = URLDownloadToFileA(NULL,
			"https://github.com/JulianAbaroa/AutoTheater-Dependencies/releases/download/FFmpeg/ffmpeg.exe",
			exePath.c_str(), BINDF_GETNEWESTVERSION, &progress);

		if (FAILED(hr))
		{
			g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Failed to download ffmpeg.exe. HRESULT: 0x%X", hr);
			DeleteFileA(exePath.c_str());
			g_pState->Infrastructure->Download->SetFFmpegInstalled(false);
			g_pState->Infrastructure->Download->SetDownloadInProgress(false);
			return;
		}

		g_pState->Infrastructure->Download->SetDownloadProgress(100.0f);
		g_pState->Infrastructure->Download->SetDownloadInProgress(false);
		g_pState->Infrastructure->Download->SetFFmpegInstalled(true);

		g_pSystem->Debug->Log("[FFmpegSystem] INFO: ffmpeg.exe downloaded.");
		}).detach();

	return true;
}

bool DownloadSystem::UninstallDependencies()
{
	std::string appData = g_pState->Infrastructure->Settings->GetAppDataDirectory();
	std::string ffmpegDir = appData + "\\FFmpeg";

	if (GetFileAttributesA(ffmpegDir.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Uninstall failed, FFmpeg directory not found.");
		return false;
	}

	std::error_code ec;
	std::filesystem::remove_all(ffmpegDir, ec);
	if (ec)
	{
		g_pSystem->Debug->Log("[FFmpegSystem] ERROR: Uninstall failed. %s", ec.message().c_str());
		return false;
	}

	g_pState->Infrastructure->Download->SetFFmpegInstalled(false);
	g_pSystem->Debug->Log("[FFmpegSystem] INFO: Dependencies uninstalled successfully.");
	return true;
}

void DownloadSystem::CancelDownload()
{
	g_pState->Infrastructure->Download->SetDownloadInProgress(false);
	g_pSystem->Debug->Log("[FFmpegSystem] WARNING: Download cancellation requested.");
}