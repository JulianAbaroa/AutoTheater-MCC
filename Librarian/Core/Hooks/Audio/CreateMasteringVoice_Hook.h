#pragma once

#include <xaudio2.h>

typedef HRESULT(__stdcall* CreateMasteringVoice_t)(
    IXAudio2*,
    IXAudio2MasteringVoice**,
    UINT32,
    UINT32,
    UINT32,
    LPCWSTR,
    const XAUDIO2_EFFECT_CHAIN*,
    AUDIO_STREAM_CATEGORY
);

namespace CreateMasteringVoice_Hook
{
    void Install();
    void Uninstall();
}