#pragma once

struct MuxerConfig
{
	int Width = 0;
	int Height = 0;
	int SampleRate = 48000;
	int Channels = 8;
	double Timebase = 90000.0;
};