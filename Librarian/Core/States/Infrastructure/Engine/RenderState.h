#pragma once

#include <d3d11.h>
#include <atomic>

class RenderState
{
public:
	ID3D11Device* GetDevice() const;
	ID3D11DeviceContext* GetContext() const;
	HWND GetHWND() const;
	ID3D11RenderTargetView* GetRTV() const;

	void SetDevice(ID3D11Device* device);
	void SetContext(ID3D11DeviceContext* context);
	void SetHWND(HWND hwnd);
	void SetRTV(ID3D11RenderTargetView* rtv);

	void CleanupRTV();
	void FullCleanup();

	bool IsImGuiInitialized() const;
	bool IsResizing() const;
	int GetWidth() const;
	int GetHeight() const;
	int GetFramerate() const;
	float GetUIScale() const;
	bool ShouldRebuildFonts() const;

	void SetImGuiInitialized(bool initialized);
	void SetResizing(bool state);
	void SetWidth(int width);
	void SetHeight(int height);
	void SetFramerate(int framerate);
	void SetUIScale(float scale);
	void ResetFontRebuild();

private:
	ID3D11Device* m_pDevice{ nullptr };
	ID3D11DeviceContext* m_pContext{ nullptr };
	HWND m_GameHWND{ nullptr };
	ID3D11RenderTargetView* m_pMainRenderTargetView{ nullptr };

	std::atomic<bool> m_IsImGuiInitialized{ false };
	std::atomic<bool> m_IsResizing{ false };
	std::atomic<int> m_Width{ 0 };
	std::atomic<int> m_Height{ 0 };
	std::atomic<int> m_Framerate{ 0 };
	std::atomic<float> m_UIScale{ 1.0f };
	std::atomic<bool> m_NeedFontRebuild{ false };
};