#include "pch.h"
#include "Core/UI/CoreUI.h"
#include "Core/Utils/CoreUtil.h"
#include "Core/States/CoreState.h"
#include "Core/Threads/CoreThread.h"
#include "External/imgui/imgui_internal.h"

void MainInterface::Draw()
{
	// Pre-render: Visibility and Input Management
	if (!g_pState->Settings.IsMenuVisible())
	{
		ImGui::GetIO().ClearInputMouse();
		ImGui::GetIO().ClearInputKeys();
		return;
	}

	// Pre-render: Position reset if requested
	this->HandleWindowReset();

	// Default window settings
	ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);

	bool open = true;
	bool isVisible = ImGui::Begin("AutoTheater - Control Panel", &open, ImGuiWindowFlags_None);

	if (!open)
	{
		g_pState->Settings.SetMenuVisible(false);
	}

	if (isVisible)
	{
		this->DrawStatusBar();
		this->DrawTabs();
	}

	ImGui::End();
}

void MainInterface::HandleWindowReset()
{
	if (!g_pState->Settings.MustResetMenu()) return;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 screenSize = viewport->Size;
	ImVec2 windowSize = ImVec2(1000, 600);

	ImGui::SetNextWindowPos(
		ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f),
		ImGuiCond_Always,
		ImVec2(0.5f, 0.5f)
	);

	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	g_pState->Settings.SetForceMenuReset(false);
}

void MainInterface::DrawStatusBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 0));

	AutoTheaterPhase currentPhase = g_pState->Lifecycle.GetCurrentPhase();
	PhaseUI ui = GetPhaseUI(currentPhase);
	bool isTheater = g_pState->Theater.IsTheaterMode();

	// Section: Phase Selector
	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("Phase:");
	ImGui::SameLine();

	if (isTheater) ImGui::BeginDisabled();
	ImGui::PushStyleColor(ImGuiCol_Text, ui.Color);

	if (ImGui::Selectable(ui.Name, false, 0, ImGui::CalcTextSize(ui.Name)))
	{
		ImGui::OpenPopup("PhaseSelectorPopup");
	}
	ImGui::PopStyleColor();

	if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip(isTheater ? "Disabled during Theater mode." : "Click to change phase.");
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
	if (ImGui::BeginPopup("PhaseSelectorPopup"))
	{
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 10));

		auto AddPhaseItem = [&](const char* label, AutoTheaterPhase phase) {
			if (ImGui::MenuItem(label, nullptr, currentPhase == phase))
			{
				g_pThread->Main.UpdateToPhase(phase);
			}
		};

		AddPhaseItem("Default", AutoTheaterPhase::Default);
		AddPhaseItem("Timeline", AutoTheaterPhase::Timeline);
		AddPhaseItem("Director", AutoTheaterPhase::Director);

		ImGui::PopStyleVar();
		ImGui::EndPopup();
	}
	ImGui::PopStyleVar();

	if (isTheater) ImGui::EndDisabled();

	// Section: Toggles
	if (currentPhase != AutoTheaterPhase::Default)
	{
		ImGui::SameLine();
		bool autoUpdatePhase = g_pState->Lifecycle.ShouldAutoUpdatePhase();
		if (ImGui::Checkbox("Auto-Update Phase", &autoUpdatePhase))
		{
			g_pState->Lifecycle.SetAutoUpdatePhase(autoUpdatePhase);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Toggle automatic transitions between Timeline and Director phases.");
		}
	}

	// Section: Engine Status
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();

	ImGui::AlignTextToFramePadding();
	ImGui::Text("Game Engine:");
	ImGui::SameLine();

	auto status = g_pState->Lifecycle.GetEngineStatus();
	ImVec4 statusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	const char* statusText = "UNKNOWN";

	switch (status) 
	{
	case EngineStatus::Waiting:
		statusText = "WAITING";
		statusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
		break;

	case EngineStatus::Running:
		statusText = "RUNNING";
		statusColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
		break;

	case EngineStatus::Destroyed:
		statusText = "DESTROYED";
		statusColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		break;
	}

	ImGui::TextColored(statusColor, statusText);

	int fps = g_pState->Render.GetFramerate();
	char fpsText[32];
	sprintf_s(fpsText, sizeof(fpsText), "%d FPS", fps);

	float textSize = ImGui::CalcTextSize(fpsText).x;
	float padding = ImGui::GetStyle().ItemSpacing.x;

	ImGui::SameLine(ImGui::GetWindowWidth() - textSize - padding - 20.0f);
	ImGui::AlignTextToFramePadding();

	ImVec4 fpsColor;
	if (fps >= 45.0f)
	{
		fpsColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else if (fps >= 30.0f)
	{
		fpsColor = ImVec4(1.0f, 0.6f, 0.0f, 1.0f);
	}
	else
	{
		fpsColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	}

	ImGui::TextColored(fpsColor, fpsText);

	ImGui::ItemSize(ImVec2(0, 10.0f));
	ImGui::PopStyleVar();
	ImGui::Separator();
	ImGui::Spacing();
}

void MainInterface::DrawTabs()
{
	static bool firstLaunch = true;

	if (!ImGui::BeginTabBar("MainTabs")) return;

	auto AddTab = [](const char* label, auto drawFn, bool forceOpen, const ImVec4* alertColor) {
		bool pushedColor = false;

		if (alertColor != nullptr)
		{
			auto now = std::chrono::steady_clock::now();
			auto elapsed = std::chrono::duration<float>(now - g_pUtil->Log.GetLastAlertTime()).count();

			ImVec4 finalColor = *alertColor;

			if (elapsed < 0.5f) 
			{
				float alpha = (sinf(elapsed * 18.84f - 1.57f) + 1.0f) * 0.5f;
				ImVec4 defaultTab = ImGui::GetStyleColorVec4(ImGuiCol_Tab);

				finalColor.x = ImLerp(defaultTab.x, finalColor.x, alpha);
				finalColor.y = ImLerp(defaultTab.y, finalColor.y, alpha);
				finalColor.z = ImLerp(defaultTab.z, finalColor.z, alpha);
			}

			ImGui::PushStyleColor(ImGuiCol_Tab, finalColor);
			ImGui::PushStyleColor(ImGuiCol_TabHovered, finalColor);
			ImGui::PushStyleColor(ImGuiCol_TabActive, finalColor);
			pushedColor = true;
		}

		ImGuiTabItemFlags flags = forceOpen ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

		if (ImGui::BeginTabItem(label, nullptr, flags))
		{
			drawFn();
			ImGui::EndTabItem();
		}

		if (pushedColor) ImGui::PopStyleColor(3);
	};

	// Primary
	AddTab("Timeline", []() { g_pUI->Timeline.Draw(); }, false, nullptr);
	AddTab("Theater", []() { g_pUI->Theater.Draw();  }, false, nullptr);
	AddTab("Director", []() { g_pUI->Director.Draw(); }, false, nullptr);
	AddTab("Settings", []() { g_pUI->Settings.Draw(); }, firstLaunch, nullptr);

	// Optional
	bool useAppData = g_pState->Settings.ShouldUseAppData();
	if (!useAppData) ImGui::BeginDisabled();

	AddTab("Replay Manager", []() { g_pUI->Replay.Draw();        }, false, nullptr);
	AddTab("Event Registry", []() { g_pUI->EventRegistry.Draw(); }, false, nullptr);
	AddTab("Capture", []() { g_pUI->Capture.Draw();       }, false, nullptr);

	if (!useAppData) ImGui::EndDisabled();

	// Logs
	const ImVec4* activeAlert = nullptr;
	static ImVec4 colorError(1.0f, 0.33f, 0.33f, 1.0f);
	static ImVec4 colorWarning(1.0f, 0.79f, 0.23f, 1.0f);

	if (g_pUtil->Log.HasUnreadError()) activeAlert = &colorError;
	else if (g_pUtil->Log.HasUnreadWarning()) activeAlert = &colorWarning;

	AddTab("Logs", []() {
		g_pUtil->Log.ClearUnreadStates();
		g_pUI->Logs.Draw();
	}, false, activeAlert);

	firstLaunch = false;
	ImGui::EndTabBar();
}


PhaseUI MainInterface::GetPhaseUI(AutoTheaterPhase phase)
{
	switch (phase)
	{
	case AutoTheaterPhase::Timeline:
		return { "Timeline", ImVec4(0.4f, 0.7f, 1.0f, 1.0f) };

	case AutoTheaterPhase::Director:
		return { "Director", ImVec4(0.4f, 1.0f, 0.4f, 1.0f) };

	case AutoTheaterPhase::Default:
	default:
		return { "Default", ImVec4(0.7f, 0.7f, 0.7f, 1.0f) };
	}
}