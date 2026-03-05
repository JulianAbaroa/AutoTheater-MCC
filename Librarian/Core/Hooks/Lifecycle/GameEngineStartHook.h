#pragma once

#include <cstdint>
#include <atomic>

// db08

class GameEngineStartHook
{
public:
	bool Install(bool silent);
	void Uninstall();

	void* GetFunctionAddress();

private:
	static void __fastcall HookedGameEngineStart(uint64_t param_1, uint64_t param_2, uint64_t* param_3);
	
	typedef void(__fastcall* GameEngineStart_t)(uint64_t, uint64_t, uint64_t*);

	static inline GameEngineStart_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };

	static constexpr uint8_t m_TheaterSignature[16] = {
		0x08, 0x03, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00
	};
};