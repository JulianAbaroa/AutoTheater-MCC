#pragma once

typedef unsigned char(__fastcall* UIBuildDynamicMessage_t)(
    int playerMask,             
    wchar_t* pTemplateStr,     
    void* pEventData,          
    unsigned int flags,        
    wchar_t* pOutBuffer       
);

namespace UIBuildDynamicMessage_Hook
{
	void Install();
	void Uninstall();
}