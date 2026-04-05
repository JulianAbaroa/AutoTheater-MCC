#include "pch.h"
#include "Core/Hooks/Audio/AudioVTableResolver.h"
#include <audiosessiontypes.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>

void* AudioVTableResolver::GetAudioClientAddress(int index)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    void* functionAddress = nullptr;

    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

    if (SUCCEEDED(hr) && pEnumerator) {
        if (SUCCEEDED(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice))) {
            if (SUCCEEDED(pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient))) {
                void** vtable = *(void***)pAudioClient;
                functionAddress = vtable[index];
                pAudioClient->Release();
            }
            pDevice->Release();
        }
        pEnumerator->Release();
    }

    if (hr != RPC_E_CHANGED_MODE) CoUninitialize();
    return functionAddress;
}

void* AudioVTableResolver::GetRenderClientAddress(int index)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    void* functionAddress = nullptr;

    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioRenderClient* pRenderClient = NULL;
    WAVEFORMATEX* pwfx = NULL;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);

    if (SUCCEEDED(hr) && pEnumerator) {
        if (SUCCEEDED(pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice))) {
            if (SUCCEEDED(pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient))) {

                pAudioClient->GetMixFormat(&pwfx);
                hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0, pwfx, NULL);

                if (SUCCEEDED(hr)) {
                    if (SUCCEEDED(pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pRenderClient))) {
                        void** vtable = *(void***)pRenderClient;
                        functionAddress = vtable[index];
                        pRenderClient->Release();
                    }
                }
                CoTaskMemFree(pwfx);
                pAudioClient->Release();
            }
            pDevice->Release();
        }
        pEnumerator->Release();
    }

    if (hr != RPC_E_CHANGED_MODE) CoUninitialize();
    return functionAddress;
}