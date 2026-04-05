#pragma once

#include <memory>

class AudioSystem;
class DownloadSystem;
class FFmpegSystem;
class ProcessSystem;
class PipeSystem;
class SyncSystem;
class VideoSystem;

class DialogSystem;
class FormatSystem;
class InputSystem;
class LifecycleSystem;
class RenderSystem;
class ScannerSystem;
class ThreadSystem;

class GallerySystem;
class PreferencesSystem;
class ReplaySystem;
class SettingsSystem;

struct CoreInfrastructureSystem
{
	CoreInfrastructureSystem();
	~CoreInfrastructureSystem();

	// Capture
	std::unique_ptr<AudioSystem> Audio;
	std::unique_ptr<DownloadSystem> Download;
	std::unique_ptr<FFmpegSystem> FFmpeg;
	std::unique_ptr<ProcessSystem> Process;
	std::unique_ptr<PipeSystem> Pipe;
	std::unique_ptr<SyncSystem> Sync;
	std::unique_ptr<VideoSystem> Video;

	// Engine
	std::unique_ptr<DialogSystem> Dialog;
	std::unique_ptr<FormatSystem> Format;
	std::unique_ptr<InputSystem> Input;
	std::unique_ptr<LifecycleSystem> Lifecycle;
	std::unique_ptr<RenderSystem> Render;
	std::unique_ptr<ScannerSystem> Scanner;
	std::unique_ptr<ThreadSystem> Thread;

	// Persistence
	std::unique_ptr<GallerySystem> Gallery;
	std::unique_ptr<PreferencesSystem> Preferences;
	std::unique_ptr<ReplaySystem> Replay;
	std::unique_ptr<SettingsSystem> Settings;
};