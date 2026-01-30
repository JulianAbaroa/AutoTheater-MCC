#pragma once

#include "Core/Common/Types/TimelineTypes.h"
#include "Core/Common/Types/UserInterfaceTypes.h"
#include <unordered_map>
#include <string>

namespace Formatting
{
    std::string ToTimestamp(float totalSeconds);

    std::string ToCompactAlpha(const std::wstring& ws);
    std::string WStringToString(const std::wstring& wstr);
    std::wstring StringToWString(const std::string& str);

    std::string EventTypeToString(EventType type);
    const std::unordered_map<EventType, EventMetadata>& GetEventDb();
}