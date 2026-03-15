#pragma once

#include <memory>

struct CoreState;
struct CoreSystem;
struct CoreHook;
struct CoreThread;
struct CoreUI;

class AppCore
{
public:
	AppCore();
	~AppCore();
	
	std::unique_ptr<CoreState> State;
	std::unique_ptr<CoreSystem> System;
	std::unique_ptr<CoreHook> Hook;
	std::unique_ptr<CoreThread> Thread;
	std::unique_ptr<CoreUI> UI;
};

extern std::unique_ptr<AppCore> g_App;