#pragma once

#include <d3d11.h>
#include <atomic>
#include <chrono>

class RenderSystem
{
public:
	void Initialize(IDXGISwapChain* pSwapChain);
	bool IsInitialized();
	void Shutdown();

	void UpdateFramerate();
	void TickCapture(IDXGISwapChain* pSwapChain);
	void CaptureFrame(IDXGISwapChain* pSwapChain);

	void BeginFrame(IDXGISwapChain* pSwapChain);
	void EndFrame();

	void* CreateTextureFromMemory(const unsigned char* data, size_t size);

private:
	void ApplyCustomStyle();

	std::atomic<int> m_FrameCount{ 0 };
	std::chrono::steady_clock::time_point m_LastFramerateTime;
	std::chrono::steady_clock::time_point m_LastCaptureTime;

	ID3D11Texture2D* m_pStagingTextures[2] = { nullptr, nullptr };
	int m_currentBufferIndex = 0;
};