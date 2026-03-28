#include "pch.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Hooks/Memory/TargetFramerateHook.h"

int TargetFramerateHook::GetCurrentFramerateValue()
{
    if (ResolveAddress())
    {
        int32_t* addr = m_pTargetFramerateAddr.load();
        if (addr)
        {
            __try 
            {
                return *addr;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                m_pTargetFramerateAddr.store(nullptr);
                return 0;
            }
        }
    }

    return 0;
}

bool TargetFramerateHook::ResolveAddress()
{
    if (m_pTargetFramerateAddr.load() != nullptr) return true;

    uintptr_t match = g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::TargetFramerate_Var);
    if (match)
    {
        int32_t relativeOffset = *(int32_t*)(match + 2);

        uintptr_t targetFramerateAddr = (match + 7) + relativeOffset;

        m_pTargetFramerateAddr.store(reinterpret_cast<int32_t*>(targetFramerateAddr));
        return true;
    }

    return false;
}

void TargetFramerateHook::Cleanup()
{
    m_pTargetFramerateAddr.store(nullptr);

    g_pSystem->Debug->Log("[TargetFramerateHook] INFO: Cleanup completed.");
}