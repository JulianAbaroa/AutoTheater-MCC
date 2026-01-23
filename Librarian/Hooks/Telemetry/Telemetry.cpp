#include "pch.h"
#include "Utils/Logger.h"
#include "Core/Scanner/Scanner.h"
#include "Hooks/Telemetry/Telemetry.h"
#include <cstdint>

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