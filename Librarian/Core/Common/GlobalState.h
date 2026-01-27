#pragma once

#include "External/imgui/imgui.h"
#include "Core/Common/Registry.h"
#include "Core/Common/Types.h"
#include <unordered_map>
#include <d3d11.h>
#include <string>
#include <queue>
#include <mutex>

struct AppState
{
	// Timeline
	std::vector<GameEvent> timeline{};
	std::atomic<bool> logGameEvents{ true };			
	std::atomic<bool> isLastEvent{ false };		
	std::atomic<size_t> processedCount{ 0 };			
	std::mutex timelineMutex;

	// Theater
	std::vector<PlayerInfo> playerList{ 16 };
	std::atomic<bool> attachThirdPersonPOV{ false };
	std::atomic<uint8_t> followedPlayerIdx{ 255 };		
	std::atomic<uint8_t> cameraAttached{ 0xFF };		
	std::atomic<bool> isTheaterMode{ false };			
	std::atomic<float*> pReplayTimeScale{ nullptr };	
	std::atomic<float*> pReplayTime{ nullptr };			
	std::atomic<uintptr_t> pReplayModule{ 0 };		
	std::atomic<float> realTimeScale{ 1.0f };
	std::atomic<double> anchorSystemTime{ 0.0f };
	std::atomic<float> anchorReplayTime{ 0.0f };
	std::mutex theaterMutex;

	// Director
	std::vector<DirectorCommand> script{};
	std::atomic<bool> directorInitialized{ false };		
	std::atomic<bool> directorHooksReady{ false };		
	std::atomic<size_t> currentCommandIndex{ 0 };		
	std::atomic<float> lastReplayTime{ 0.0f };			
	std::mutex directorMutex;

	// Input
	std::atomic<GameInput> nextInput{ { InputContext::Unknown, InputAction::Unknown } };
	std::atomic<bool> inputProcessing{ false };			
	std::queue<InputRequest> inputQueue{};
	std::mutex inputMutex;

	// Debug
	std::vector<std::string> debugLogs{}	;
	std::mutex logMutex;

	// Game Events Registry
	std::unordered_map<std::wstring, EventInfo> eventRegistry = g_EventRegistry;

	// Configuration
	std::atomic<bool> showMenu{ true };
	std::atomic<bool> forceMenuReset{ false };
	std::atomic<bool> freezeMouse{ true };
	std::string baseDirectory{};
	std::string loggerPath{};
	std::string filmPath{};
	std::mutex configMutex;

	// D3D11
	ID3D11Device* pDevice{ nullptr };
	ID3D11DeviceContext* pContext{ nullptr };
	HWND gameHWND{ nullptr };
	ID3D11RenderTargetView* pMainRenderTargetView{ nullptr };

	// Project
	std::atomic<bool> running{ false };					
	std::atomic<HMODULE> handleModule{ nullptr };		
	std::atomic<EngineStatus> engineStatus{ EngineStatus::Idle };
	std::atomic<Phase> currentPhase{ Phase::Default };

	void SaveTimeline(std::string replayName);
	void LoadTimeline(std::string replayName);

	void SaveToAppData();
	void LoadFromAppData();
};

extern AppState* g_pState; 
#define g_State (*g_pState)