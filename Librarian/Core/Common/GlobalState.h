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

// Estructura que contiene todos los datos globales de AutoTheater.
struct AppState
{
	// Timeline Data

	// Vector utilizado para guardar todos los GameEvents registrados en una repeticion.
	std::vector<GameEvent> timeline{};

	// Flag utilizada para definir cuando loguear los GameEvents.
	std::atomic<bool> logGameEvents{ true };	

	// Flag utilizada para decidir cuando parar de guardar GameEvents dentro de la Timeline.
	std::atomic<bool> isLastEvent{ false };

	// Contador que contiene todos los GameEvents que ya han sido logueados.
	std::atomic<size_t> processedCount{ 0 };		

	// Mutex para modificar la Timeline de forma segura.
	std::mutex timelineMutex;



	// Theater Engine State

	// Vector utilizado para guardar la informacion de los jugadores actuales.
	std::vector<PlayerInfo> playerList{ 16 };

	// Flag utilizada forzar el seguimiento de la camara del jugador que se esta espectando.
	std::atomic<bool> attachThirdPersonPOV{ false };

	// El index actual del jugador que esta siendo seguido por el especador.
	std::atomic<uint8_t> followedPlayerIdx{ 255 };	

	// Byte que determina el estado de la camara del especador en cuanto a estar en modo libre o en modo bloqueado.
	std::atomic<uint8_t> cameraAttached{ 0xFF };	

	// Flag utilizada para saber si el modo teatro esta activado.
	std::atomic<bool> isTheaterMode{ false };		

	// Puntero hacia la Timescale del modo teatro.
	std::atomic<float*> pReplayTimeScale{ nullptr };	

	// Puntero hacia el ReplayTime del modo teatro. 
	std::atomic<float*> pReplayTime{ nullptr };		

	// Puntero hacia el struct del ReplayModule.
	std::atomic<uintptr_t> pReplayModule{ 0 };	

	// Float que guarda la estimacion calculada del Timescale.
	std::atomic<float> realTimeScale{ 1.0f };

	// Float que guarda el ultimo SystemTime.
	std::atomic<double> anchorSystemTime{ 0.0f };

	// Float que guarda el ultimo ReplayTime.
	std::atomic<float> anchorReplayTime{ 0.0f };

	// Mutex para modificar la PlayerList de form segura.
	std::mutex theaterMutex;



	// Director Data
	
	// Vector utilizado para guardar todos los comandos del Director generados a partir de la Timeline.
	std::vector<DirectorCommand> script{};

	// Flag que determina si el director fue inicializado.
	std::atomic<bool> directorInitialized{ false };		

	// Flag que determina si los hooks del director estan listos.
	std::atomic<bool> directorHooksReady{ false };	

	// Contador del indice del comando actual.
	std::atomic<size_t> currentCommandIndex{ 0 };	

	// Float que contiene el ultimo ReplayTime.
	std::atomic<float> lastReplayTime{ 0.0f };		

	// Mutex para modificar el script de forma segura.
	std::mutex directorMutex;

	// Input Injection
	
	// GameInput utilizado para definir el siguiente input a injectar.
	std::atomic<GameInput> nextInput{ { InputContext::Unknown, InputAction::Unknown } };

	// Flag que determina si un input inyectado esta siendo procesado.
	std::atomic<bool> inputProcessing{ false };		

	// Queue utilizado para poder manejar multiples inputs en orden.
	std::queue<InputRequest> inputQueue{};

	// Mutex para modificar el inputQueue de forma segura.
	std::mutex inputMutex;



	// Debugging
	
	// Vector utilizado para guardar los logs que tienen que ser escritos a la LogTab.
	std::vector<std::string> debugLogs{};
	std::mutex logMutex;



	// Replay Manager
	
	// Vector que funciona como cache para las replays guardadas.
	std::vector<SavedReplay> savedReplaysCache;

	// Flag que determina si la lista de replays deberia de ser actualizada.
	std::atomic<bool> refreshReplayList{ true };

	// Objeto que contiene la metadata del ultimo replay abierto.
	CurrentFilmMetadata currentMetadata{};

	// String que contiene el path hacia el anterior replay abierto.
	std::string lastProcessedPath{};

	// String que contiene el ultime replay hash utilizado.
	std::string activeReplayHash{};

	// String que contiene el path hacia el ultimo replay abierto.
	std::string filmPath{};

	// Mutex para modificar variables del ReplayManager de forma segura.
	std::mutex replayManagerMutex;



	// UI and Configuration
	
	// Flag que determina si AutoTheater tiene permitido utilizar el AppData.
	std::atomic<bool> useAppData{ false };

	// Flag que determina si el menu de ImGui deberia de mostrarse.
	std::atomic<bool> showMenu{ true };

	// Flag que determina si el menu de ImGui deberia de resetearse.
	std::atomic<bool> forceMenuReset{ false };

	// Flag que determina si se deberia de congelar el movimiento del mouse en el juego cuando el menu de ImGui esta abierto.
	std::atomic<bool> freezeMouse{ true };

	// String que contiene el directorio base donde se encuentra el DLL de AutoTheater.
	std::string baseDirectory{};

	// String que contiene el directorio hacia el AppData del usuario (AppData/Local/AutoTheater).
	std::string appDataDirectory{};

	// String que contiene el directorio hacia las repeticiones del modo teatro de Halo Reach.
	std::string mccTempMovieDirectory{};

	// String que contiene el path hacia el archivo de texto del logger.
	std::string loggerPath{};
	
	// Game Metadata

	// Objeto que contiene todos los eventos registrados, con sus pesos.
	std::unordered_map<std::wstring, EventInfo> eventRegistry = g_EventRegistry;

	// // Mutex para modificar variables del 'UI and Configuration' de forma segura.
	std::mutex configMutex;



	// Direct3D 11 Render State
	ID3D11Device* pDevice{ nullptr };
	ID3D11DeviceContext* pContext{ nullptr };
	HWND gameHWND{ nullptr };
	ID3D11RenderTargetView* pMainRenderTargetView{ nullptr };



	// System Lifecycle

	// Flag que determina si AutoTheater esta corriendo.
	std::atomic<bool> running{ false };					

	// Direccion de memoria base del ejecutable principal (MCC-Win64-Shipping.exe).
	std::atomic<HMODULE> handleModule{ nullptr };

	// Estado actual del game engine Blam! (Halo Reach).
	std::atomic<EngineStatus> engineStatus{ EngineStatus::Awaiting };

	// La fase actual de AutoTheater (Default, Timeline, Director).
	std::atomic<AutoTheaterPhase> currentPhase{ AutoTheaterPhase::Default };

	// Flag que determina si se actualiza de fase automaticamente al salir de una repeticion.
	std::atomic<bool> autoUpdatePhase{ true };

	/** @brief Mechanism to block the unload process until threads finish cleanup. */
	std::condition_variable shutdownCV;
	std::mutex shutdownMutex;
};

/** @brief Global pointer to the singleton AppState instance. */
extern AppState* g_pState; 

/** @brief Helper macro for cleaner access to the global state. */
#define g_State (*g_pState)