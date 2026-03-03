#pragma once

class DirectorTab
{
public:
	void Draw();

private:
	void DrawDirectorSystemStatus();
	void DrawDirectorProgress(bool& autoScroll);
};