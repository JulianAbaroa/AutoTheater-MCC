#pragma once

#include <atomic>

struct DebugSnap {
	uint64_t obj;                // Dirección original (para referencia)
	uint32_t seqNum;             // ID de secuencia
	PlayerState stateSnapshot;   // COPIA completa de los datos
	char playerName[16];
};

// Tamaño del buffer (potencia de 2 para que el % sea eficiente)
extern const uint32_t SNAP_BUFFER_SIZE;

// El buffer circular
extern DebugSnap g_SnapBuffer[];

extern std::atomic<uint32_t> g_WritePos;
extern std::atomic<uint32_t> g_ReadPos;

typedef void(__fastcall* FilmPlayerState_Decode_t)(
	uint64_t filmContext,
	PlayerObject* playerObjectPtr
);

namespace FilmPlayerState_Decode_Hook
{
	void Install();
	void Uninstall();
}