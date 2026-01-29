#pragma once

/**
 * @file GlobalState.h
 * @brief Centralized Synchronized State Manager.
 * * This file defines the AppState structure, which serves as the "Single Source of Truth"
 * for the entire mod. It facilitates communication between specialized threads
 * and ensures data consistency across the Timeline, Director, and Input systems.
 */

#include "External/imgui/imgui.h"
#include "Core/Common/Registry.h"
#include "Core/Common/Types.h"
#include <condition_variable>
#include <unordered_map>
#include <d3d11.h>
#include <string>
#include <queue>
#include <mutex>

/**
 * @struct AppState
 * @brief Global container for the mod's operational data.
 * * @note All members marked with std::atomic are thread-safe for a simple read/write.
 * @note Complex containers (std::vector, std::queue) must be protected by their
 * respective std::mutex using std::lock_guard.
 */
struct AppState
{
	// Timeline Data
	/** @brief List of processed events captured from the game engine. */
	std::vector<GameEvent> timeline{};

	/** @brief Enables or disables the capturing of the game events. */
	std::atomic<bool> logGameEvents{ true };	

	/** @brief Flag indicating the end of the replay, and disables the acquisition of more game events. */
	std::atomic<bool> isLastEvent{ false };

	/** @brief Keeps track of how many events have been processed by the LogThread */
	std::atomic<size_t> processedCount{ 0 };			
	std::mutex timelineMutex;



	// Theater Engine State
	/** @brief Snapshot of the current player data (Positions, Weapons, Names, etc.). */
	std::vector<PlayerInfo> playerList{ 16 };

	/** @brief Locks/Unlocks the third person POV on Director phase. */
	std::atomic<bool> attachThirdPersonPOV{ false };

	/** @brief The index of the player that the spectator is currently following. */
	std::atomic<uint8_t> followedPlayerIdx{ 255 };	

	/** @brief Current spectator third-person camera mode (e.g., Attached, Detached) */
	std::atomic<uint8_t> cameraAttached{ 0xFF };	

	/** @brief True if the engine is currently in Theater mode. */
	std::atomic<bool> isTheaterMode{ false };		

	/** @brief Pointer to the engine's playback speed variable. */
	std::atomic<float*> pReplayTimeScale{ nullptr };	

	/** @brief Pointer to the engine's current elapsed replay time. */
	std::atomic<float*> pReplayTime{ nullptr };		

	/** @brief Base address of the module responsible for replay data. */
	std::atomic<uintptr_t> pReplayModule{ 0 };	

	/** @brief State for calculating the actual playback speed via differential sampling. */
	std::atomic<float> realTimeScale{ 1.0f };
	std::atomic<double> anchorSystemTime{ 0.0f };
	std::atomic<float> anchorReplayTime{ 0.0f };
	std::mutex theaterMutex;



	// Director Data
	/** @brief Sequence of commands generated to automate the replay playback. */
	std::vector<DirectorCommand> script{};

	/** @brief State flag for the Director subsystem. */
	std::atomic<bool> directorInitialized{ false };		

	/** @brief True when all necessary engine hooks for the Director are active. */
	std::atomic<bool> directorHooksReady{ false };	

	/** @brief Iterator index for the generated script execution. */
	std::atomic<size_t> currentCommandIndex{ 0 };	

	/** @brief Cache of the last known replay timestamp to detect jumps/rewinds. */
	std::atomic<float> lastReplayTime{ 0.0f };			
	std::mutex directorMutex;

	// Input Injection
	/** @brief The next raw input to be injected into the game engine. */
	std::atomic<GameInput> nextInput{ { InputContext::Unknown, InputAction::Unknown } };

	/** @brief Lock flag to prevent overlapping input requests. */
	std::atomic<bool> inputProcessing{ false };		

	/** @brief Queue  of pending input actions (e.g., Switch Player, Toggle UI). */
	std::queue<InputRequest> inputQueue{};
	std::mutex inputMutex;



	// Debugging
	/** @brief Rolling buffer of strings for the in-game ImGui console. */
	std::vector<std::string> debugLogs{};
	std::mutex logMutex;



	// Game Metadata
	/** @brief Map for translating engine events IDs to human-readable Info/Weights. */
	std::unordered_map<std::wstring, EventInfo> eventRegistry = g_EventRegistry;



	// Replay Manager
	/** @brief Local cache of the replays saved to disk to avoid constant file readings in each frame. */
	std::vector<SavedReplay> savedReplaysCache;

	/** @brief Flag indicating whether the repetition directory should be rescanned to update the list. */
	std::atomic<bool> refreshReplayList{ true };

	/** @brief Stores extended metadata (author & info) of the movie file that the game has open. */
	CurrentFilmMetadata currentMetadata{};

	/** @brief Stores the path of the last processed file to detect changes and avoid unnecessary hash recalculations. */
	std::string lastProcessedPath{};

	/** @brief The current replay hash, used to ensure a replay is not initiated with the wrong timeline */
	std::string activeReplayHash{};

	/** @brief Full path to the replay file (.mov / .film) currently detected in the game's memory. */
	std::string filmPath{};
	std::mutex replayManagerMutex;



	// UI and Configuration
	/** @brief Determines if the users allowed AutoTheater to use AppData */
	std::atomic<bool> useAppData{ false };


	/** @brief Toggles the visibility of the ImGui Overlay. */
	std::atomic<bool> showMenu{ true };

	/** @brief Signal to re-initialize ImGui layout and position. */
	std::atomic<bool> forceMenuReset{ false };

	/** @brief Freezes the in-game mouse when the ImGui overlay is opened. */
	std::atomic<bool> freezeMouse{ true };

	/** @brief Filesystem environment and storage paths for logs and replay data. */
	std::string baseDirectory{};
	std::string appDataDirectory{};
	std::string mccTempDirectory{};
	std::string loggerPath{};
	std::mutex configMutex;



	// Direct3D 11 Render State
	ID3D11Device* pDevice{ nullptr };
	ID3D11DeviceContext* pContext{ nullptr };
	HWND gameHWND{ nullptr };
	ID3D11RenderTargetView* pMainRenderTargetView{ nullptr };



	// System Lifecycle
	/** @brief Global kill-switch for all worker threads. */
	std::atomic<bool> running{ false };					

	/** @brief HMODULE of this DLL, used for self-injection. */
	std::atomic<HMODULE> handleModule{ nullptr };

	/** @brief Current lifecycle state of the Blam! engine. */
	std::atomic<EngineStatus> engineStatus{ EngineStatus::Idle };

	/** @brief The operational mode of AutoTheater (Timeline, Director, Default). */
	std::atomic<AutoTheaterPhase> currentPhase{ AutoTheaterPhase::Default };

	/** @brief Toggles automated phase transitions (e.g., Timeline -> Director). */
	std::atomic<bool> autoUpdatePhase{ true };

	/** @brief Mechanism to block the unload process until threads finish cleanup. */
	std::condition_variable shutdownCV;
	std::mutex shutdownMutex;
};

/** @brief Global pointer to the singleton AppData instance. */
extern AppState* g_pState; 

/** @brief Helper macro for cleaner access to the global state. */
#define g_State (*g_pState)