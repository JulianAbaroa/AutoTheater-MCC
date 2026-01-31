#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/GlobalState.h"
#include <sstream>
#include <fstream>
#include <iomanip>

// Se encarga de loguear tanto a disco como a la LogTab.
void Logger::LogAppend(const char* format) {
	std::string loggerPathCopy;

	// 1. Crea una copia del logger path.
	{
		std::lock_guard<std::mutex> lock(g_pState->ConfigMutex);
		loggerPathCopy = g_pState->LoggerPath;
	}

	// 2. Abre el archivo y se va a la ultima linea escrita.
	std::ofstream ofs(loggerPathCopy, std::ios::app);

	// 3. Obtiene y formatea el tiempo del sistema.
	SYSTEMTIME system_time;
	GetLocalTime(&system_time);
	ofs << "[" << std::setw(4) << system_time.wYear << "-"
		<< std::setfill('0') << std::setw(2) << system_time.wMonth << "-"
		<< std::setw(2) << system_time.wDay << " "
		<< std::setw(2) << system_time.wHour << ":"
		<< std::setw(2) << system_time.wMinute << ":"
		<< std::setw(2) << system_time.wSecond << "] "
		<< format << "\r\n";
	ofs.close();

	// 4. Añade el log actual al vector de logs.
	{
		std::lock_guard lock(g_pState->LogMutex);
		g_pState->DebugLogs.push_back(std::string(format));
		if (g_pState->DebugLogs.size() > 500)
		{
			g_pState->DebugLogs.erase(g_pState->DebugLogs.begin());
		}
	}
}