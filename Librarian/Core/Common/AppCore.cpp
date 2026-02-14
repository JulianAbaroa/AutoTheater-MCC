#include "pch.h"
#include "Core/Common/AppCore.h"

std::unique_ptr<AppCore> g_App = nullptr;

CoreSystem* g_pSystem = nullptr;
CoreState* g_pState = nullptr;
CoreUI* g_pUI = nullptr;

AppCore::AppCore()
{
	System = std::make_unique<CoreSystem>();
	State = std::make_unique<CoreState>();
	UI = std::make_unique<CoreUI>();

	g_pSystem = System.get();
	g_pState = State.get();
	g_pUI = UI.get();
}

AppCore::~AppCore()
{
	g_pUI = nullptr;
	g_pSystem = nullptr;
	g_pState = nullptr;
}