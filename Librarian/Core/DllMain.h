#pragma once

#include <atomic>
#include <string>

enum LibrarianPhase
{
    Start, 
    BuildTimeline,
    ExecuteDirector,
    End
};

extern LibrarianPhase g_CurrentPhase;
extern std::atomic<bool> g_Running;
extern std::string g_BaseDirectory;
extern HMODULE g_HandleModule;