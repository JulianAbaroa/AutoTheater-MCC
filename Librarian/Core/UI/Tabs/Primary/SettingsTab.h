#pragma once

class SettingsTab
{
public:
	void Draw();

private:
	void DrawHotkey(const char* label, const char* keys, const char* tooltip);
	void DrawPathField(const char* label, const std::string& path);
};