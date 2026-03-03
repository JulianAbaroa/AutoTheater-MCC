#pragma once

#include "Core/Threads/MainThread.h"
#include "Core/Threads/DirectorThread.h"
#include "Core/Threads/InputThread.h"
#include "Core/Threads/LogThread.h"
#include "Core/Threads/CaptureThread.h"

struct CoreThread
{
	MainThread Main;
	DirectorThread Director;
	InputThread Input;
	LogThread Log;
	CaptureThread Capture;
};

extern CoreThread* g_pThread;