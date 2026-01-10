#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Systems/Theater.h"
#include "FilmPlayerState_Decode_Hook.h"
#include <atomic>

// =========================================================
// CONFIGURACIÓN DE DEBUG
// =========================================================
// Tecla para continuar después de una pausa (F5)
#define KEY_RESUME VK_F5 
// Tecla para activar/desactivar el "Modo Breakpoint" (F6)
#define KEY_TOGGLE_DEBUG VK_F6 

static bool g_DebugModeEnabled = true; // Empieza activado por defecto?
static PlayerState g_LastPlayerState;
static bool g_FirstRun = true;

// Rangos de memoria a ignorar (Posición, Rotación, Cámara) para no pausar por movimiento
struct MemRange { uint32_t start; uint32_t end; };
const std::vector<MemRange> BLACKLIST = {
	{0x04, 0x0F},   // WorldPosition
	{0x2C, 0x33},   // Rotation
	{0x22C, 0x237}  // LookVector / Camera
};

// =========================================================

FilmPlayerState_Decode_t original_FilmPlayerState_Decode = nullptr;
std::atomic<bool> g_FilmPlayerState_Decode_Hook_Installed;
void* g_FilmPlayerState_Decode_Address;

const uint32_t SNAP_BUFFER_SIZE = 4096;
DebugSnap g_SnapBuffer[SNAP_BUFFER_SIZE];
std::atomic<uint32_t> g_WritePos{ 0 };
std::atomic<uint32_t> g_ReadPos{ 0 };

bool IsValidPtr(void* ptr) {
	// Verifica que no sea nulo y que no esté en el rango de memoria reservada del sistema (Kernel)
	return (ptr != nullptr && (uintptr_t)ptr > 0x10000 && (uintptr_t)ptr < 0x00007FFFFFFEFFFF);
}

// Función auxiliar para comparar estados ignorando ruido
bool HasRelevantChanges(PlayerState* currentState, PlayerState* lastState) {
	unsigned char* pCurrent = reinterpret_cast<unsigned char*>(currentState);
	unsigned char* pLast = reinterpret_cast<unsigned char*>(lastState);
	size_t size = sizeof(PlayerState);

	for (size_t i = 0; i < size; i++) {
		// Verificar si este byte está en la lista negra
		bool isBlacklisted = false;
		for (const auto& range : BLACKLIST) {
			if (i >= range.start && i <= range.end) {
				isBlacklisted = true;
				break;
			}
		}
		if (isBlacklisted) continue;

		// Si encontramos un cambio en un byte NO ignorado
		if (pCurrent[i] != pLast[i]) {
			return true;
		}
	}
	return false;
}

__declspec(noinline) void SafeCopySnapshot(PlayerObject* pObj, PlayerState* pState, const char* playerName)
{
	__try {
		if (!pObj || !pState) return;

		uint32_t seqNum = *(uint32_t*)((uint8_t*)pObj + 0x14);
		uint32_t writeIdx = g_WritePos.load();
		uint32_t pos = writeIdx % 4096;

		g_SnapBuffer[pos].obj = (uint64_t)pState;
		g_SnapBuffer[pos].seqNum = seqNum;

		if (playerName) {
			strncpy_s(g_SnapBuffer[pos].playerName, playerName, _TRUNCATE);
		}
		else {
			g_SnapBuffer[pos].playerName[0] = '\0';
		}

		memcpy(&g_SnapBuffer[pos].stateSnapshot, pState, sizeof(PlayerState));

		g_WritePos.store(writeIdx + 1);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {}
}

void hkFilmPlayerState_Decode(uint64_t filmContext, PlayerObject* playerObjectPtr)
{
	// --- LÓGICA DE IDENTIFICACIÓN ---
	std::string playerName = "Unknown";
	std::string tag = "";
	PlayerObject* pLocalObj = playerObjectPtr;

	if (pLocalObj != nullptr)
	{
		wchar_t* rawTag = reinterpret_cast<wchar_t*>(reinterpret_cast<uintptr_t>(pLocalObj) + 0x12A);
		std::wstring wsTag;
		for (int i = 0; i < 4; i++) {
			if (iswprint(rawTag[i]) && rawTag[i] != 0) {
				wsTag += rawTag[i];
			}
			else break;
		}
		tag = std::string(wsTag.begin(), wsTag.end());

		uint32_t playerIndex = pLocalObj->playerIndex;
		if (g_PlayerList.count(playerIndex))
		{
			playerName = g_PlayerList[playerIndex].name;
		}
	}

	// --- LLAMADA ORIGINAL ---
	original_FilmPlayerState_Decode(filmContext, playerObjectPtr);

	// --- LÓGICA DE CAPTURA ---
	if (playerName == "PlaceHolder021" && tag == "021")
	{
		if (pLocalObj == nullptr || !IsValidPtr(pLocalObj)) return;

		void* pStateRaw = pLocalObj->playerState;
		if (!IsValidPtr(pStateRaw)) return;

		PlayerState* playerState = static_cast<PlayerState*>(pStateRaw);

		// 1. Guardar Snapshot para el worker thread (Logger)
		SafeCopySnapshot(pLocalObj, playerState, playerName.c_str());

		// 2. LÓGICA DE "BREAKPOINT"
		// Togglear modo debug con F6
		if (GetAsyncKeyState(KEY_TOGGLE_DEBUG) & 1) {
			g_DebugModeEnabled = !g_DebugModeEnabled;
			logAppend(g_DebugModeEnabled ? "[DEBUG] Breakpoint Mode ENABLED" : "[DEBUG] Breakpoint Mode DISABLED");
			Beep(1000, 200); // Beep agudo confirmación
		}

		if (g_DebugModeEnabled)
		{
			if (!g_FirstRun)
			{
				// Comparamos el estado actual con el anterior (excluyendo floats de movimiento)
				if (HasRelevantChanges(playerState, &g_LastPlayerState))
				{
					// ¡CAMBIO DETECTADO!

					// Aviso sonoro para que mires la pantalla
					Beep(750, 100);

					logAppend(">>> BREAKPOINT HIT: Cambio relevante detectado. Juego PAUSADO. Presiona F5 para continuar.");

					// === EL BUCLE DE CONGELAMIENTO ===
					// Esto detiene el hilo principal del juego. Todo se congela.
					while (true)
					{
						// Si presionamos F5, rompemos el bucle y dejamos que el juego siga al siguiente frame
						if (GetAsyncKeyState(KEY_RESUME) & 0x8000) {
							break;
						}

						// Sleep suave para no quemar la CPU mientras esperamos tu input
						Sleep(10);
					}
				}
			}

			// Actualizamos el estado anterior para la próxima vuelta
			memcpy(&g_LastPlayerState, playerState, sizeof(PlayerState));
			g_FirstRun = false;
		}
	}
}

void FilmPlayerState_Decode_Hook::Install()
{
	if (g_FilmPlayerState_Decode_Hook_Installed.load()) return;

	void* Blam_OpenFile_Address = (void*)(FilmPlayerState_Decode_RVA + (uintptr_t)GetHaloReachModuleBaseAddress());

	if (!Blam_OpenFile_Address)
	{
		logAppend("Failed to obtain the address of FilmPlayerState_Decode()");
		return;
	}

	g_FilmPlayerState_Decode_Address = Blam_OpenFile_Address;

	if (MH_CreateHook(g_FilmPlayerState_Decode_Address, &hkFilmPlayerState_Decode, reinterpret_cast<LPVOID*>(&original_FilmPlayerState_Decode)) != MH_OK)
	{
		logAppend("Failed to create FilmPlayerState_Decode hook");
		return;
	}

	if (MH_EnableHook(g_FilmPlayerState_Decode_Address) != MH_OK)
	{
		logAppend("Failed to enalbe FilmPlayerState_Decode hook");
		return;
	}

	g_FilmPlayerState_Decode_Hook_Installed.store(true);
	logAppend("FilmPlayerState_Decode hook installed");
}

void FilmPlayerState_Decode_Hook::Uninstall()
{
	if (!g_FilmPlayerState_Decode_Hook_Installed.load()) return;

	MH_DisableHook(g_FilmPlayerState_Decode_Address);
	MH_RemoveHook(g_FilmPlayerState_Decode_Address);

	g_FilmPlayerState_Decode_Hook_Installed.store(false);
	logAppend("FilmPlayerState_Decode hook uninstalled");
}