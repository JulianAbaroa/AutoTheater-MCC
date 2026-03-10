#include "pch.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"

ID3D11Device* RenderState::GetDevice() const 
{ 
	return m_pDevice; 
}

ID3D11DeviceContext* RenderState::GetContext() const
{
	return m_pContext;
}

HWND RenderState::GetHWND() const
{
	return m_GameHWND;
}

ID3D11RenderTargetView* RenderState::GetRTV() const
{
	return m_pMainRenderTargetView;
}


void RenderState::SetDevice(ID3D11Device* device) 
{ 
	m_pDevice = device; 
}

void RenderState::SetContext(ID3D11DeviceContext* context) 
{ 
	m_pContext = context; 
}

void RenderState::SetHWND(HWND hwnd) 
{ 
	m_GameHWND = hwnd; 
}

void RenderState::SetRTV(ID3D11RenderTargetView* rtv) 
{ 
	m_pMainRenderTargetView = rtv; 
}


bool RenderState::IsReady() const 
{
	return m_pDevice != nullptr && m_pContext != nullptr && m_pMainRenderTargetView != nullptr;
}

void RenderState::CleanupRTV() 
{
	if (m_pMainRenderTargetView) 
	{
		m_pMainRenderTargetView->Release();
		m_pMainRenderTargetView = nullptr;
	}
}

void RenderState::FullCleanup() 
{
	CleanupRTV();

	m_pDevice = nullptr;
	m_pContext = nullptr;
	m_GameHWND = nullptr;
}


bool RenderState::IsImGuiInitialized()
{
	return m_IsImGuiInitialized.load();
}

void RenderState::SetImGuiInitialized(bool initialized)
{
	m_IsImGuiInitialized.store(initialized);
}

bool RenderState::IsResizing() const
{
	return m_IsResizing.load();
}

void RenderState::SetResizing(bool state)
{
	m_IsResizing.store(state);
}


int RenderState::GetWidth()
{
	return m_Width.load();
}

void RenderState::SetWidth(int width)
{
	m_Width.store(width);
}

int RenderState::GetHeight()
{
	return m_Height.load();
}

void RenderState::SetHeight(int height)
{
	m_Height.store(height);
}


int RenderState::GetFramerate() const
{
	return m_Framerate.load();
}

void RenderState::SetFramerate(int framerate)
{
	m_Framerate.store(framerate);
}

float RenderState::GetUIScale() const { return m_UIScale.load(); }
void RenderState::SetUIScale(float scale) { m_UIScale.store(scale); m_NeedFontRebuild.store(true); }

bool RenderState::ShouldRebuildFonts() const { return m_NeedFontRebuild.load(); }
void RenderState::ResetFontRebuild() { m_NeedFontRebuild.store(false); }