
// Blam! uses TLS (Thread Local Storage) to safely manage data on multi-threaded contexts.
// Only a thread with a TLS index initialized by Blam! can "see" or access this data.
// This is why these functions must be called within the scope of a Blam! function hook (or a game-managed thread).

#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Hooks/Scanner/Scanner.h"
#include "Core/Hooks/Data/Telemetry/Telemetry.h"

// Returns the pointer to the PlayerTable within the game's memory.
// This table acts as the primary data structure for session-specific player data,
// including nicknames, service IDs, and real-time state: world position, rotation,
// look vector, and handles for primary/secondary weapons, biped, and objective items.
// Note: this function relies on Thread Local Storage (TLS) to retrieve the correct
// telemetry context for the current execution thread.
// Reference: See "Core/Common/Types/BlamTypes.h" to find the specific structs.
uintptr_t Telemetry::GetTelemetryPlayerTable()
{
	__try
	{
		uintptr_t tlsArray = (uintptr_t)__readgsqword(0x58);

        uintptr_t match = Scanner::FindPattern(Signatures::TelemetryIdModifier);

        if (match)
        {
            int32_t relativeOffset = *(int32_t*)(match + 2);
            uintptr_t telemetryIdAddr = (match + 6) + relativeOffset;
		    uint32_t telemetryIdx = *(uint32_t*)(telemetryIdAddr);

            if (telemetryIdx <= 1000)
            {
		        uintptr_t threadContext = *(uintptr_t*)(tlsArray + (static_cast<unsigned long long>(telemetryIdx) * 8));
                
                if (threadContext) {
		            uintptr_t telemetryData = *(uintptr_t*)(threadContext + 0x18);

                    if (telemetryData)
                    {
                        uintptr_t playerTable = *(uintptr_t*)(telemetryData + 0x50);
		                return playerTable; 
                    }
                }
            }
        }

        return 0;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0;
	}
}

// Returns the pointer to the Global Object Table (Indirection Table).
// This table acts as a lookup array where each 24-byte (0x18) entry maps a 
// networked 'Handle' to a physical memory address.
// Resolution logic: The lower 16 bits of a Handle (0xFFFF) serve as the index
// into this table, while the upper 16 bits function as a 'Salt' for versioning.
// Each entry contains a 64-bit pointer to the actual entity data (Bipeds, Weapons, etc.).
// Capacity: Probably 1024 slots, supporting the 650 Forge-item limit
// plus dynamic entities like projectiles and player assets.
uintptr_t Telemetry::GetTelemetryObjectTable()
{
    __try
    {
        uintptr_t tlsArray = (uintptr_t)__readgsqword(0x58);

        uintptr_t match = Scanner::FindPattern(Signatures::TelemetryIdModifier);

        if (match)
        {
            int32_t relativeOffset = *(int32_t*)(match + 2);
            uintptr_t telemetryIdAddr = (match + 6) + relativeOffset;
            uint32_t telemetryIdx = *(uint32_t*)(telemetryIdAddr);

            if (telemetryIdx <= 1000) {
                uintptr_t threadContext = *(uintptr_t*)(tlsArray + (static_cast<unsigned long long>(telemetryIdx) * 8));

                if (threadContext) {
                    uintptr_t telemetryData = *(uintptr_t*)(threadContext + 0x10);

                    if (telemetryData) {
                        uintptr_t objectTable = *(uintptr_t*)(telemetryData + 0x50);
                        return objectTable;
                    }
                }
            }
        }

        return 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) 
    { 
        return 0;
    }
}