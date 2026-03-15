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
class ThreadSystem;
class FormatSystem;
class DialogSystem;
class DownloadSystem;
class SyncSystem;

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
	std::unique_ptr<ThreadSystem> Thread;
	std::unique_ptr<FormatSystem> Format;
	std::unique_ptr<DialogSystem> Dialog;
	std::unique_ptr<DownloadSystem> Download;
	std::unique_ptr<SyncSystem> Sync;
};