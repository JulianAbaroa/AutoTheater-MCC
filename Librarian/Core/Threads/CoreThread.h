#pragma once

#include <memory>

class MainThread;
class DirectorThread;
class InputThread;
class CaptureThread;
class WriterThread;

// Main container for the application's threads.
struct CoreThread
{
	CoreThread();
	~CoreThread();

	std::unique_ptr<MainThread> Main;
	std::unique_ptr<DirectorThread> Director;
	std::unique_ptr<InputThread> Input;
	std::unique_ptr<CaptureThread> Capture;
	std::unique_ptr<WriterThread> Writer;
};

extern CoreThread* g_pThread;