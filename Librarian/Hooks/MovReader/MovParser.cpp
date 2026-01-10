#include "pch.h"
#include "Utils/Logger.h"
#include "Hooks/MovReader/MovTypes.h"
#include "Hooks/MovReader/MovParser.h"
#include <iomanip>
#pragma comment(lib, "ws2_32.lib")

void MovParser::GetHeaderSizes(const std::string& filePath)
{
	HANDLE hFile = CreateFileA(
		filePath.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		std::stringstream ss;
		ss << "Windows error when opening: " << error;
		Logger::LogAppend(ss.str().c_str());
		return;
	}

	SetFilePointer(hFile, 48, NULL, FILE_BEGIN);

	BLFChunkHeader header;
	DWORD bytesRead;

	int safetyCounter = 0;
	while (ReadFile(hFile, &header, sizeof(BLFChunkHeader), &bytesRead, NULL) && bytesRead == sizeof(BLFChunkHeader))
	{
		if (++safetyCounter > 100) break; 
		
		uint32_t chunkSize = _byteswap_ulong(header.size);
		std::string magic(header.magic, 4);

		if (!isprint((unsigned char)magic[0]) || !isprint((unsigned char)magic[1]))
		{
			Logger::LogAppend("WARNING: End of valid chunks reached or desynchronization");
			break;
		}

		uint32_t bytes = chunkSize;
		double displaySize = static_cast<double>(bytes);
		std::string unit = "Bytes";

		if (bytes >= 1073741824) { // 1GB
			displaySize = bytes / 1073741824.0;
			unit = "GB";
		}
		else if (bytes >= 1048576) { // 1MB
			displaySize = bytes / 1048576.0;
			unit = "MB";
		}
		else if (bytes >= 1024) { // 1KB
			displaySize = bytes / 1024.0;
			unit = "KB";
		}

		std::stringstream ss;
		ss << "Block: [" << magic << "] Size: "
			<< std::fixed << std::setprecision(2) << displaySize << " " << unit;
		Logger::LogAppend(ss.str().c_str());

		if (magic == "flmd")
		{
			if (chunkSize == 0xFFFFFFFF || chunkSize > 0x7FFFFFFF) {
				Logger::LogAppend("-> FLMD detected as Stream Final (Infinite size). Stopping header parsing");
				break;
			}

			uint32_t dataSize = chunkSize - sizeof(BLFChunkHeader);
			Logger::LogAppend("-> FLMD located. Skipping compressed data...");
			SetFilePointer(hFile, dataSize, NULL, FILE_CURRENT);
		}
		else
		{
			uint32_t jumpSize = chunkSize - sizeof(BLFChunkHeader);

			if (jumpSize > 1000000) {
				Logger::LogAppend("Error: Absurdly large block jump, aborting");
				break;
			}
			SetFilePointer(hFile, jumpSize, NULL, FILE_CURRENT);
		}
	}

	Logger::LogAppend("Reading complete");
	CloseHandle(hFile);
}