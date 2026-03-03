#pragma once

class TheaterTab
{
public:
	void Draw();

private:
	void DrawTheaterStatus();
	void DrawPlaybackControls(bool& autoScroll);
};