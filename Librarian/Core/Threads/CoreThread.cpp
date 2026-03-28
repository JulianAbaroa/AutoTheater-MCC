#include "pch.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Threads/Domain/MainThread.h"
#include "Core/Threads/Domain/DirectorThread.h"
#include "Core/Threads/Infrastructure/InputThread.h"
#include "Core/Threads/Infrastructure/CaptureThread.h"
#include "Core/Threads/Infrastructure/WriterThread.h"

CoreThread::CoreThread()
{
	Main = std::make_unique<MainThread>();
	Director = std::make_unique<DirectorThread>();
	Input = std::make_unique<InputThread>();
	Capture = std::make_unique<CaptureThread>();
	Writer = std::make_unique<WriterThread>();
}

CoreThread::~CoreThread() = default;