#pragma once

#include <memory>

struct CoreState;
struct CoreUtil;
struct CoreThread;
struct CoreSystem;
struct CoreUI;
struct CoreHook;

class AppCore
{
public:
	AppCore();
	~AppCore();

	std::unique_ptr<CoreState> State;
	std::unique_ptr<CoreUtil> Util;
	std::unique_ptr<CoreThread> Thread;
	std::unique_ptr<CoreSystem> System;
	std::unique_ptr<CoreUI> UI;
	std::unique_ptr<CoreHook> Hook;
};

extern std::unique_ptr<AppCore> g_App;