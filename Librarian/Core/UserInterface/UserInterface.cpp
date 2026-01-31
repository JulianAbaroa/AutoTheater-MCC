#include "pch.h"
#include "Utils/Formatting.h"
#include "Core/Common/GlobalState.h"
#include "Core/Threads/MainThread.h"
#include "Core/UserInterface/UserInterface.h"
#include "Core/UserInterface/Tabs/Logs/LogsTab.h"
#include "Core/UserInterface/Tabs/Primary/TimelineTab.h"
#include "Core/UserInterface/Tabs/Primary/TheaterTab.h"
#include "Core/UserInterface/Tabs/Primary/DirectorTab.h"
#include "Core/UserInterface/Tabs/Primary/ConfigurationTab.h"
#include "Core/UserInterface/Tabs/Optional/ReplayManagerTab.h"
#include "Core/UserInterface/Tabs/Optional/EventRegistryTab.h"
#include "External/imgui/imgui_internal.h"
#include <algorithm>
#include <vector>

static PhaseUI GetPhaseUI(AutoTheaterPhase phase)
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


static void DrawStatusBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 0));

	AutoTheaterPhase currentPhase = g_pState->CurrentPhase.load();
	PhaseUI ui = GetPhaseUI(currentPhase);
	bool isTheater = g_pState->IsTheaterMode.load();

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
				MainThread::UpdateToPhase(phase);
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
		bool autoUpdatePhase = g_pState->AutoUpdatePhase.load();
		if (ImGui::Checkbox("Auto-Update Phase", &autoUpdatePhase))
		{
			g_pState->AutoUpdatePhase.store(autoUpdatePhase);	
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

	auto status = g_pState->EngineStatus.load();
	ImVec4 statusColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
	const char* statusText = "UNKNOWN";

	switch (status) {
	case EngineStatus::Awaiting:  
		statusText = "AWAITING..."; 
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

	ImGui::ItemSize(ImVec2(0, 10.0f));

	ImGui::PopStyleVar();
	ImGui::Separator();
	ImGui::Spacing();
}

void HandleWindowReset()
{
	if (!g_pState->ForceMenuReset.load()) return;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 screenSize = viewport->Size;
	ImVec2 windowSize = ImVec2(1000, 600);

	ImGui::SetNextWindowPos(
		ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f),
		ImGuiCond_Always,
		ImVec2(0.5f, 0.5f)
	);

	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	g_pState->ForceMenuReset.store(false);
}

void DrawTabs()
{
	if (!ImGui::BeginTabBar("MainTabs")) return;

	static bool firstLaunch = true;

	auto AddTab = [](const char* label, void (*drawFn)(), bool forceOpen = false) {
		ImGuiTabItemFlags flags = forceOpen ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;

		if (ImGui::BeginTabItem(label, nullptr, flags))
		{
			drawFn();
			ImGui::EndTabItem();
		}
	};

	// Primary Tabs
	AddTab("Timeline", TimelineTab::Draw);
	AddTab("Theater", TheaterTab::Draw);
	AddTab("Director", DirectorTab::Draw);
	AddTab("Configuration", ConfigurationTab::Draw, firstLaunch);

	// Optional Tabs
	bool useAppData = g_pState->UseAppData.load();
	if (!useAppData) ImGui::BeginDisabled();
	AddTab("Replay Manager", ReplayManagerTab::Draw);
	AddTab("Event Registry", EventRegistryTab::Draw);
	if (!useAppData) ImGui::EndDisabled();

	// Log Tab
	AddTab("Logs", LogsTab::Draw);

	firstLaunch = false;
	ImGui::EndTabBar();
}

void UserInterface::DrawMainInterface()
{
	// 1. Pre-render: Visibility and Input Management
	if (!g_pState->ShowMenu.load())
	{
		ImGui::GetIO().ClearInputMouse();
		ImGui::GetIO().ClearInputKeys();
		return;
	}

	// 2. Pre-render: Position reset if requested
	HandleWindowReset();

	// 3. Default window settings
	ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);

	bool open = true;
	bool isVisible = ImGui::Begin("AutoTheater - Control Panel", &open, ImGuiWindowFlags_None);

	if (!open)
	{
		g_pState->ShowMenu.store(false);
	}

	if (isVisible)
	{
		DrawStatusBar();
		DrawTabs();
	}

	ImGui::End();
}