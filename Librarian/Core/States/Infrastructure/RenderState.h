#pragma once

#include <d3d11.h>

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

private:
	ID3D11Device* pDevice{ nullptr };
	ID3D11DeviceContext* pContext{ nullptr };
	HWND GameHWND{ nullptr };
	ID3D11RenderTargetView* pMainRenderTargetView{ nullptr };
};