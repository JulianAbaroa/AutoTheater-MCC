#pragma once

#include "Core/Common/Registry.h"
#include "Core/Common/Types.h"
#include <unordered_map>
#include <string>
#include <queue>
#include <mutex>

struct AppState
{
	// Timeline
	std::vector<GameEvent> timeline;
	std::atomic<bool> logGameEvents{ true };			
	std::atomic<bool> isLastEvent{ false };				
	std::atomic<size_t> processedCount{ 0 };			
	std::mutex timelineMutex;

	// Theater
	std::vector<PlayerInfo> playerList{ 16 };
	std::atomic<uint8_t> followedPlayerIdx{ 255 };		
	std::atomic<uint8_t> cameraAttached{ 0xFF };		
	std::atomic<bool> isTheaterMode{ false };			
	std::atomic<float*> pReplayTimeScale{ nullptr };	
	std::atomic<float*> pReplayTime{ nullptr };			
	std::atomic<uintptr_t> pReplayModule{ 0 };			
	std::mutex theaterMutex;

	// Director
	std::vector<DirectorCommand> script;
	std::atomic<bool> directorInitialized{ false };		
	std::atomic<size_t> currentCommandIndex{ 0 };		
	std::atomic<float> lastReplayTime{ 0.0f };			
	std::mutex directorMutex;

	// Input
	std::atomic<GameInput> nextInput{ { InputContext::Unknown, InputAction::Unknown } };
	std::atomic<bool> inputProcessing{ false };			
	std::queue<InputRequest> inputQueue;
	std::mutex inputMutex;

	// Configuration
	std::unordered_map<std::wstring, EventInfo> eventRegistry = g_EventRegistry;
	std::string baseDirectory;
	std::string loggerPath;
	std::string filmPath;
	std::mutex configMutex;

	// Other
	std::atomic<bool> running{ false };					
	std::atomic<HMODULE> handleModule{ nullptr };		

	std::atomic<bool> engineHooksReady{ false };		
	std::atomic<bool> gameEngineDestroyed{ false };		
	std::atomic<Phase> currentPhase{ Phase::Default };	

	void SaveTimeline(std::string replayName);
	void LoadTimeline(std::string replayName);

	void SaveToAppData();
	void LoadFromAppData();
};

extern AppState g_State;