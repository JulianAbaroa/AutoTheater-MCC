#pragma once

class DownloadSystem
{
public:
	bool DownloadDependencies();
	bool UninstallDependencies();
	void CancelDownload();

private:
	class DownloadProgress;
};