#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Common/AppCore.h"
#include "CreateMasteringVoice_Hook.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "External/minhook/include/MinHook.h"

CreateMasteringVoice_t original_CreateMasteringVoice = nullptr;
std::atomic<bool> g_CreateMasteringVoice_Hook_Installed{ false };
void* g_CreateMasteringVoice_Address = nullptr;

HRESULT __stdcall hkCreateMasteringVoice(
    IXAudio2* pInstance,
    IXAudio2MasteringVoice** ppMasteringVoice,
    UINT32 InputChannels,
    UINT32 InputSampleRate,
    UINT32 Flags,
    LPCWSTR DeviceId,
    const XAUDIO2_EFFECT_CHAIN* pEffectChain,
    AUDIO_STREAM_CATEGORY StreamCategory)
{
    g_pState->Audio.SetEngineInstance(pInstance);
    return original_CreateMasteringVoice(pInstance, ppMasteringVoice, InputChannels, InputSampleRate, Flags, DeviceId, pEffectChain, StreamCategory);
}

void CreateMasteringVoice_Hook::Install()
{
	if (g_CreateMasteringVoice_Hook_Installed.load()) return;

    void* methodAddress = (void*)Scanner::FindPattern(Signatures::CreateMasteringVoice_Wrapper);
    if (!methodAddress)
    {
        Logger::LogAppend("Failed to obtain the address of CreateMasteringVoice()");
        return;
    }

    g_CreateMasteringVoice_Address = methodAddress;

    if (MH_CreateHook(g_CreateMasteringVoice_Address, &hkCreateMasteringVoice, reinterpret_cast<LPVOID*>(&original_CreateMasteringVoice)) != MH_OK)
    {
        Logger::LogAppend("Failed to create CreateMasteringVoice hook");
        return;
    }

    if (MH_EnableHook(g_CreateMasteringVoice_Address) != MH_OK)
    {
        Logger::LogAppend("Failed to enable CreateMasteringVoice hook");
        return;
    }

    g_CreateMasteringVoice_Hook_Installed.store(true);
    Logger::LogAppend("CreateMasteringVoice wrapper hook installed");
}

void CreateMasteringVoice_Hook::Uninstall()
{
	if (!g_CreateMasteringVoice_Hook_Installed.load()) return;

	MH_DisableHook(g_CreateMasteringVoice_Address);
	MH_RemoveHook(g_CreateMasteringVoice_Address);

	g_CreateMasteringVoice_Hook_Installed.store(false);
	Logger::LogAppend("CreateMasteringVoice hook uninstalled");
}