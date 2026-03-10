#include "pch.h"
#include "Core/Hooks/Window/CoreWindowHook.h"
#include "Core/Hooks/Window/WndProcHook.h"

CoreWindowHook::CoreWindowHook()
{
	WndProc = std::make_unique<WndProcHook>();
}

CoreWindowHook::~CoreWindowHook() = default;