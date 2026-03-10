#pragma once

#include <memory>

class AudioSystem;
class FFmpegSystem;
class VideoSystem;
class InputSystem;
class LifecycleSystem;
class RenderSystem;
class ScannerSystem;
class GallerySystem;
class PreferencesSystem;
class ReplaySystem;
class SettingsSystem;

struct CoreInfrastructureSystem
{
	CoreInfrastructureSystem();
	~CoreInfrastructureSystem();

	std::unique_ptr<AudioSystem> Audio;
	std::unique_ptr<FFmpegSystem> FFmpeg;
	std::unique_ptr<VideoSystem> Video;
	std::unique_ptr<InputSystem> Input;
	std::unique_ptr<LifecycleSystem> Lifecycle;
	std::unique_ptr<RenderSystem> Render;
	std::unique_ptr<ScannerSystem> Scanner;
	std::unique_ptr<GallerySystem> Gallery;
	std::unique_ptr<PreferencesSystem> Preferences;
	std::unique_ptr<ReplaySystem> Replay;
	std::unique_ptr<SettingsSystem> Settings;
};