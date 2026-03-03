#pragma once

#include <cstdint>
#include <atomic>

class BlamOpenFileHook
{
public:
	void Install();
	void Uninstall();

private:
	static void HookedBlamOpenFile(
		long long fileContext, uint32_t accessFlags, uint32_t* translatedStatus);

	typedef void(__fastcall* BlamOpenFile_t)(
		long long fileContext, uint32_t accessFlags, uint32_t* translatedStatus);

	static inline BlamOpenFile_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};