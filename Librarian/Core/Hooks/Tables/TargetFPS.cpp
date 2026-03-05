#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Hooks/Tables/TargetFPS.h"

int TargetFPS::GetCurrentFPSValue()
{
    if (ResolveAddress())
    {
        int32_t* addr = m_pTargetFPSAddr.load();
        if (addr)
        {
            return *addr;
        }
    }

    return 0;
}

bool TargetFPS::ResolveAddress()
{
    if (m_pTargetFPSAddr.load() != nullptr) return true;

    uintptr_t match = g_pSystem->Scanner.FindPattern(Signatures::TargetFPS_Var);
    if (match)
    {
        int32_t relativeOffset = *(int32_t*)(match + 2);

        uintptr_t targetFPSAddr = (match + 7) + relativeOffset;

        m_pTargetFPSAddr.store(reinterpret_cast<int32_t*>(targetFPSAddr));
        return true;
    }

    return false;
}