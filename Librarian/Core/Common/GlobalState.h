#pragma once

#include "Core/Common/EventRegistry.h"
#include "External/imgui/imgui.h"
#include "Core/Common/Types/InputTypes.h"
#include "Core/Common/Types/TimelineTypes.h"
#include "Core/Common/Types/DirectorTypes.h"
#include "Core/Common/Types/AutoTheaterTypes.h"
#include "Core/Common/Types/UserInterfaceTypes.h"
#include <condition_variable>
#include <unordered_map>
#include <d3d11.h>
#include <string>
#include <queue>
#include <mutex>

// Structure containing all global data for AutoTheater.
struct AppState
{
	// Timeline Data

	// Vector used to store all GameEvents recorded during the Timeline phase.
	std::vector<GameEvent> Timeline{};

	// Flag used to determine when to log GameEvents.
	std::atomic<bool> LogGameEvents{ true };	

	// Flag used to decide when to stop saving GameEvents into the Timeline.
	std::atomic<bool> IsLastEvent{ false };

	// Counter containing the number of GameEvents already logged.
	std::atomic<size_t> ProcessedCount{ 0 };		

	// Mutex for thread-safe Timeline modification.
	std::mutex TimelineMutex;



	// Theater Engine State

	// Vector used to store current player information.
	std::vector<PlayerInfo> PlayerList{ 16 };

	// Flag used to force camera attachment to the spectated player's POV.
	std::atomic<bool> AttachThirdPersonPOV{ false };

	// Current index of the player being followed by the spectator.
	std::atomic<uint8_t> FollowedPlayerIdx{ 255 };	

	// Byte determining the spectator camera state (free mode vs. locked mode).
	std::atomic<uint8_t> CameraAttached{ 0xFF };	

	// Flag used to track if theater mode is currently active.
	std::atomic<bool> IsTheaterMode{ false };		

	// Pointer to the theater mode Timescale.
	std::atomic<float*> pReplayTimeScale{ nullptr };	

	// Pointer to the theater mode ReplayTime.
	std::atomic<float*> pReplayTime{ nullptr };		

	// Pointer to the ReplayModule structure.
	std::atomic<uintptr_t> pReplayModule{ 0 };	

	// Float storing the calculated estimation of the Timescale.
	std::atomic<float> RealTimeScale{ 1.0f };

	// Float storing the last SystemTime anchor.
	std::atomic<double> AnchorSystemTime{ 0.0f };

	// Float storing the last ReplayTime anchor.
	std::atomic<float> AnchorReplayTime{ 0.0f };

	// Mutex for thread-safe PlayerList modification.
	std::mutex TheaterMutex;



	// Director Data
	
	// Vector used to store all Director commands generated from the Timeline.
	std::vector<DirectorCommand> Script{};

	// Flag determining if the director has been initialized.
	std::atomic<bool> DirectorInitialized{ false };		

	// Flag determining if the director hooks are ready.
	std::atomic<bool> DirectorHooksReady{ false };	

	// Index counter for the current active command.
	std::atomic<size_t> CurrentCommandIndex{ 0 };	

	// Float containing the last recorded ReplayTime.
	std::atomic<float> LastReplayTime{ 0.0f };		

	// Mutex for thread-safe script modification.
	std::mutex DirectorMutex;



	// Input Injection
	
	// GameInput used to define the next input to be injected.
	std::atomic<GameInput> NextInput{ { InputContext::Unknown, InputAction::Unknown } };

	// Flag determining if an injected input is currently being processed.
	std::atomic<bool> InputProcessing{ false };		

	// Queue used to handle multiple inputs in sequential order.
	std::queue<InputRequest> InputQueue{};

	// Mutex for thread-safe inputQueue modification.
	std::mutex InputMutex;



	// Debugging
	
	// Vector used to store logs that need to be written to the LogTab.
	std::vector<std::string> DebugLogs{};
	std::mutex LogMutex;



	// Replay Manager
	
	// Vector serving as a cache for saved replays.
	std::vector<SavedReplay> SavedReplaysCache;

	// Flag determining if the replay list should be refreshed.
	std::atomic<bool> RefreshReplayList{ true };

	// Object containing metadata for the currently opened replay.
	CurrentFilmMetadata CurrentMetadata{};

	// String containing the path to the previously opened replay.
	std::string LastProcessedPath{};

	// String containing the last used replay hash.
	std::string ActiveReplayHash{};

	// String containing the path to the current replay file.
	std::string FilmPath{};

	// Mutex for thread-safe ReplayManager variable modification.
	std::mutex ReplayManagerMutex;



	// UI and Configuration
	
	// Flag determining if AutoTheater is allowed to use AppData storage.
	std::atomic<bool> UseAppData{ false };

	// Flag determining if the ImGui menu should be displayed.
	std::atomic<bool> ShowMenu{ true };

	// Flag determining if the ImGui menu should be forced to reset.
	std::atomic<bool> ForceMenuReset{ false };

	// Flag determining if game mouse movement should be frozen when the ImGui menu is open.
	std::atomic<bool> FreezeMouse{ true };

	// String containing the base directory where the AutoTheater DLL is located.
	std::string BaseDirectory{};

	// String containing the path to the user's AppData directory (AppData/Local/AutoTheater).
	std::string AppDataDirectory{};

	// String containing the path to the Halo Reach theater temporary movie directory.
	std::string MCCTempMovieDirectory{};

	// String containing the path to the logger text file.
	std::string LoggerPath{};
	
	// Game Metadata

	// Object containing all registered events and their respective weights.
	std::unordered_map<std::wstring, EventInfo> EventRegistry = g_EventRegistry;

	// Mutex for thread-safe 'UI and Configuration' variable modification.	
	std::mutex ConfigMutex;



	// Direct3D 11 Render State
	ID3D11Device* pDevice{ nullptr };
	ID3D11DeviceContext* pContext{ nullptr };
	HWND GameHWND{ nullptr };
	ID3D11RenderTargetView* pMainRenderTargetView{ nullptr };



	// System Lifecycle

	// Flag determining if AutoTheater is currently running.
	std::atomic<bool> Running{ false };

	// Base memory address of the main executable (MCC-Win64-Shipping.exe).
	std::atomic<HMODULE> HandleModule{ nullptr };

	// Current status of the Blam! game engine (Halo Reach).
	std::atomic<EngineStatus> EngineStatus{ EngineStatus::Awaiting };

	// Current phase of AutoTheater (Default, Timeline, Director).
	std::atomic<AutoTheaterPhase> CurrentPhase{ AutoTheaterPhase::Default };

	// Flag determining if the phase should automatically update upon exiting a replay.
	std::atomic<bool> AutoUpdatePhase{ true };

	// Mechanism to block the unload process until threads finish cleanup.
	std::condition_variable ShutdownCV;
	std::mutex ShutdownMutex;
};

// Global pointer to the singleton AppState instance.
extern AppState* g_pState; 

// Helper macro for cleaner access to the global state.
#define g_State (*g_pState)