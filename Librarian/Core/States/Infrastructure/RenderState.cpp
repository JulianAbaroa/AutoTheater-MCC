#include "pch.h"
#include "Core/States/Infrastructure/RenderState.h"

ID3D11Device* RenderState::GetDevice() const 
{ 
	return pDevice; 
}

ID3D11DeviceContext* RenderState::GetContext() const
{
	return pContext;
}

HWND RenderState::GetHWND() const
{
	return GameHWND;
}

ID3D11RenderTargetView* RenderState::GetRTV() const
{
	return pMainRenderTargetView;
}


void RenderState::SetDevice(ID3D11Device* device) 
{ 
	pDevice = device; 
}

void RenderState::SetContext(ID3D11DeviceContext* context) 
{ 
	pContext = context; 
}

void RenderState::SetHWND(HWND hwnd) 
{ 
	GameHWND = hwnd; 
}

void RenderState::SetRTV(ID3D11RenderTargetView* rtv) 
{ 
	pMainRenderTargetView = rtv; 
}


bool RenderState::IsReady() const 
{
	return pDevice != nullptr && pContext != nullptr && pMainRenderTargetView != nullptr;
}

void RenderState::CleanupRTV() 
{
	if (pMainRenderTargetView) 
	{
		pMainRenderTargetView->Release();
		pMainRenderTargetView = nullptr;
	}
}

void RenderState::FullCleanup() 
{
	CleanupRTV();

	pDevice = nullptr;
	pContext = nullptr;
	GameHWND = nullptr;
}