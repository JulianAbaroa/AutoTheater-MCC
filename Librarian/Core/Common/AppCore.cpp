#include "pch.h"
#include "Core/Common/AppCore.h"

std::unique_ptr<AppCore> g_App = nullptr;

CoreSystem* g_pSystem = nullptr;
CoreState* g_pState = nullptr;

AppCore::AppCore()
{
	State = std::make_unique<CoreState>();
	System = std::make_unique<CoreSystem>();

	g_pState = State.get();
	g_pSystem = System.get();
}

AppCore::~AppCore()
{
	g_pState = nullptr;
}