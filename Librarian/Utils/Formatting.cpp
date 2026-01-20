#include "pch.h"
#include "Utils/Formatting.h"

std::string Formatting::ToTimestamp(float totalSeconds) {
    if (totalSeconds < 0) totalSeconds = 0;

    int hours = static_cast<int>(totalSeconds) / 3600;
    int minutes = (static_cast<int>(totalSeconds) % 3600) / 60;
    int seconds = static_cast<int>(totalSeconds) % 60;
    int milliseconds = static_cast<int>((totalSeconds - static_cast<int>(totalSeconds)) * 100);

    char buffer[32];
    if (hours > 0) {
        snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%02d", hours, minutes, seconds, milliseconds);
    }
    else {
        snprintf(buffer, sizeof(buffer), "%02d:%02d.%02d", minutes, seconds, milliseconds);
    }
    return std::string(buffer);
}

std::string Formatting::ToCompactAlpha(const std::wstring& ws) {
    std::string s;
    s.reserve(ws.length());
    for (wchar_t wc : ws) {
        if (wc > 0 && wc < 127 && iswprint(wc) && wc != L' ') {
            s += static_cast<char>(std::tolower(static_cast<unsigned char>(wc)));
        }
    }
    return s;
}

std::wstring Formatting::ToCompactAlphaW(const std::string& s_in) {
    std::wstring ws;
    ws.reserve(s_in.length());
    for (unsigned char c : s_in) {
        if (c > 0 && c < 127 && isprint(c) && c != ' ') {
            ws += static_cast<wchar_t>(std::tolower(c));
        }
    }
    return ws;
}

std::wstring Formatting::ToCompactAlphaW(const std::wstring& ws) {
    std::wstring s;
    s.reserve(ws.length());
    for (wchar_t wc : ws) {
        if (wc > 0 && wc < 127 && iswprint(wc) && wc != L' ') {
            s += static_cast<wchar_t>(std::tolower(static_cast<unsigned char>(wc)));
        }
    }
    return s;
}

std::string Formatting::WStringToString(const wchar_t* wstr) {
    if (!wstr) return "";
    std::string result;

    for (size_t i = 0; i < 256 && wstr[i] != L'\0'; ++i) {
        if (iswprint(wstr[i])) {
            result += static_cast<char>(wstr[i]);
        }
        else {
            result += '?';
        }
    }
    return result;
}