#pragma once

#include <memory>

class WndProcHook;

struct CoreWindowHook
{
	CoreWindowHook();
	~CoreWindowHook();

	std::unique_ptr<WndProcHook> WndProc;
};