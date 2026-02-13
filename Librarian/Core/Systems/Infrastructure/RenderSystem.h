#pragma once

#include <d3d11.h>


class RenderSystem
{
public:
	void Initialize(IDXGISwapChain* pSwapChain);
	bool IsInitialized();
	void Shutdown();

	void BeginFrame(IDXGISwapChain* pSwapChain);
	void EndFrame();

private:
	void ApplyCustomStyle();
};