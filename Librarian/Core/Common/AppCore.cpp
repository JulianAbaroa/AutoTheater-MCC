#include "pch.h"
#include "Core/Common/AppCore.h"

#include "Core/States/CoreState.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/Threads/CoreThread.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/UI/CoreUI.h"
#include "Core/Hooks/CoreHook.h"

CoreState* g_pState = nullptr;
CoreUtil* g_pUtil = nullptr;
CoreThread* g_pThread = nullptr;
CoreSystem* g_pSystem = nullptr;
CoreUI* g_pUI = nullptr;
CoreHook* g_pHook = nullptr;

std::unique_ptr<AppCore> g_App = nullptr;

AppCore::AppCore()
{
    State = std::make_unique<CoreState>();
    g_pState = State.get();

    Util = std::make_unique<CoreUtil>();
    g_pUtil = Util.get();

    Thread = std::make_unique<CoreThread>();
    g_pThread = Thread.get();

    System = std::make_unique<CoreSystem>();
    g_pSystem = System.get();

    UI = std::make_unique<CoreUI>();
    g_pUI = UI.get();

    Hook = std::make_unique<CoreHook>();
    g_pHook = Hook.get();
}

AppCore::~AppCore()
{
	g_pState = nullptr;
	g_pUtil = nullptr;
	g_pThread = nullptr;
	g_pSystem = nullptr;
	g_pUI = nullptr;
	g_pHook = nullptr;
}