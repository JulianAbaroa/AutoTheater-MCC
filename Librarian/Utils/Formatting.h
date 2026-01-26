#pragma once

#include "Core/Common/Types.h"
#include <unordered_map>
#include <string>

namespace Formatting
{
    std::string ToTimestamp(float totalSeconds);

    std::string ToCompactAlpha(const std::wstring& ws);
    std::wstring ToCompactAlphaW(const std::wstring& ws);
    std::wstring ToCompactAlphaW(const std::string& s_in);

    std::string WStringToString(const wchar_t* wstr);

    std::string EventTypeToString(EventType type);
    const std::unordered_map<EventType, EventMetadata>& GetEventDb();
}