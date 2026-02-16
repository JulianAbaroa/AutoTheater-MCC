#pragma once

#include <d3d11.h>
#include <atomic>

struct RenderState
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

	bool IsReady() const;
	void CleanupRTV();
	void FullCleanup();

	bool IsImGuiInitialized();
	void SetImGuiInitialized(bool initialized);

	bool IsResizing() const;
	void SetResizing(bool state);

private:
	ID3D11Device* m_pDevice{ nullptr };
	ID3D11DeviceContext* m_pContext{ nullptr };
	HWND m_GameHWND{ nullptr };
	ID3D11RenderTargetView* m_pMainRenderTargetView{ nullptr };

	std::atomic<bool> m_IsImGuiInitialized{ false };
	std::atomic<bool> m_IsResizing{ false };
};