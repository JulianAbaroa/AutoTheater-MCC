#pragma once

#include <fstream>
#include <atomic>

class BuildGameEventHook
{
public:
    void Install();
    void Uninstall();

private:
    static unsigned char HookedBuildGameEvent(
        int playerMask, wchar_t* pTemplateStr, void* pEventData,
        unsigned int flags, wchar_t* pOutBuffer);

    static void PrintNewEvent(wchar_t* pTemplateStr, void* pEventData, 
        wchar_t* pOutBuffer, std::wstring currentTemplate, float currentTime);

    static void PrintRawEvent(wchar_t* pOutBuffer);

    typedef unsigned char(__fastcall* BuildGameEvent_t)(int playerMask, wchar_t* pTemplateStr,
        void* pEventData, unsigned int flags, wchar_t* pOutBuffer);

    static inline BuildGameEvent_t m_OriginalFunction = nullptr;
    std::atomic<void*> m_FunctionAddress{ nullptr };
    std::atomic<bool> m_IsHookInstalled{ false };

};