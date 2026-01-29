#include "pch.h"
#include "Utils/Formatting.h"
#include "Core/Common/GlobalState.h"
#include "Core/Threads/MainThread.h"
#include "Core/UserInterface/UserInterface.h"
#include "Core/UserInterface/Tabs/LogsTab.h"
#include "Core/UserInterface/Tabs/TimelineTab.h"
#include "Core/UserInterface/Tabs/TheaterTab.h"
#include "Core/UserInterface/Tabs/DirectorTab.h"
#include "Core/UserInterface/Tabs/ConfigurationTab.h"
#include "Core/UserInterface/Tabs/ReplayManagerTab.h"
#include "Core/UserInterface/Tabs/EventRegistryTab.h"
#include "External/imgui/imgui_internal.h"
#include "External/imgui/imgui.h"
#include <algorithm>
#include <vector>

static void DrawStatusBar()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 0));

	ImGui::Text("Phase:");
	ImGui::SameLine();

	AutoTheaterPhase current = g_pState->currentPhase.load();
	ImVec4 phaseColor;
	std::string phaseName;

	switch (current)
	{
	case AutoTheaterPhase::Timeline: 
		phaseColor = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
		phaseName = "Timeline";
		break;

	case AutoTheaterPhase::Director:
		phaseColor = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
		phaseName = "Director";
		break;

	default: 
		phaseColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
		phaseName = "Default";
	}

	bool isTheater = g_pState->isTheaterMode.load();
	if (isTheater) ImGui::BeginDisabled();

	ImGui::PushStyleColor(ImGuiCol_Text, phaseColor);
	if (ImGui::Selectable(phaseName.c_str(), false, 0, ImGui::CalcTextSize(phaseName.c_str())))
	{
		ImGui::OpenPopup("PhaseSelectorPopup");
	}
	ImGui::PopStyleColor();

	if (!ImGui::IsPopupOpen("PhaseSelectorPopup"))
	{
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			if (isTheater)
			{
				ImGui::SetTooltip("Manual phase change disabled during Theater mode.");
			}
			else
			{
				ImGui::SetTooltip("Click to change phase manually.");
			}
		}
	}

	if (ImGui::BeginPopup("PhaseSelectorPopup"))
	{
		if (ImGui::MenuItem("Default", nullptr, current == AutoTheaterPhase::Default))
		{
			MainThread::UpdateToPhase(AutoTheaterPhase::Default);
		}

		if (ImGui::MenuItem("Timeline", nullptr, current == AutoTheaterPhase::Timeline))
		{
			MainThread::UpdateToPhase(AutoTheaterPhase::Timeline);
		}

		if (ImGui::MenuItem("Director", nullptr, current == AutoTheaterPhase::Director))
		{
			MainThread::UpdateToPhase(AutoTheaterPhase::Director);
		}

		ImGui::EndPopup();
	}

	if (isTheater) ImGui::EndDisabled();

	ImGui::SameLine();
	bool autoUpdatePhase = g_pState->autoUpdatePhase.load();
	if (ImGui::Checkbox("Auto-Update Phase", &autoUpdatePhase))
	{
		g_pState->autoUpdatePhase.store(autoUpdatePhase);
	}

	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();

	ImGui::Text("Game Engine:");
	ImGui::SameLine();
	switch (g_pState->engineStatus.load())
	{
	case EngineStatus::Idle:
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "AWAITING..."); 
		break;

	case EngineStatus::Running:
		ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "ONLINE");  
		break;

	case EngineStatus::Destroyed:
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "OFFLINE");  
		break;
	}

	ImGui::PopStyleVar();
	ImGui::Separator();
	ImGui::Spacing();
}

void UserInterface::DrawMainInterface()
{
	if (g_pState->forceMenuReset.load())
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		
		ImVec2 screenSize = viewport->Size;
		ImVec2 windowSize = ImVec2(1000, 600);

		ImGui::SetNextWindowPos(
			ImVec2(screenSize.x * 0.5f, screenSize.y * 0.5f),
			ImGuiCond_Always,
			ImVec2(0.5f, 0.5f)
		);

		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

		g_pState->forceMenuReset.store(false);
	}

	if (!g_pState->showMenu.load())
	{
		ImGui::GetIO().ClearInputMouse();
		ImGui::GetIO().ClearInputKeys();
		return;
	}

	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
	{
		ImGui::GetIO().MouseDown[0] = false;
	}

	ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);

	bool open = g_pState->showMenu.load();
	if (ImGui::Begin("AutoTheater - Control Panel", &open))
	{
		g_pState->showMenu.store(open);

		DrawStatusBar();

		if (ImGui::BeginTabBar("MainTabs"))
		{
			if (ImGui::BeginTabItem("Timeline"))
			{
				TimelineTab::Draw();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Theater"))
			{
				TheaterTab::Draw();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Director"))
			{
				DirectorTab::Draw();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Configuration"))
			{
				ConfigurationTab::Draw();
				ImGui::EndTabItem();
			}

			bool useAppData = g_pState->useAppData.load();

			if (!useAppData) ImGui::BeginDisabled();
			if (ImGui::BeginTabItem("Replay Manager"))
			{
				ReplayManagerTab::Draw();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Event Registry"))
			{
				EventRegistryTab::Draw();
				ImGui::EndTabItem();
			}
			if (!useAppData) ImGui::EndDisabled();

			if (ImGui::BeginTabItem("Logs"))
			{
				LogsTab::Draw();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	
	ImGui::End();
}