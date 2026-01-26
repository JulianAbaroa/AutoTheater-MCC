#include "pch.h"
#include "Utils/Formatting.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/Tabs/LogsTab.h"
#include "Core/UserInterface/Tabs/TimelineTab.h"
#include "Core/UserInterface/Tabs/TheaterTab.h"
#include "Core/UserInterface/Tabs/DirectorTab.h"
#include "Core/UserInterface/Tabs/ConfigurationTab.h"
#include "Core/UserInterface/UserInterface.h"
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
	Phase current = g_pState->currentPhase.load();
	ImVec4 phaseColor;
	std::string phaseName;

	switch (current)
	{
	case Phase::BuildTimeline: 
		phaseColor = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
		phaseName = "Build Timeline";
		break;

	case Phase::ExecuteDirector:
		phaseColor = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
		phaseName = "Execute Director";
		break;

	default: 
		phaseColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
		phaseName = "Default";
	}

	ImGui::TextColored(phaseColor, "[ %s ]", phaseName.c_str());

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
		ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_Always);
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
	if (ImGui::Begin("AutoTheater - Control Panel", &open, ImGuiWindowFlags_MenuBar))
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
			if (ImGui::BeginTabItem("Event Registry"))
			{
				EventRegistryTab::Draw();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Configuration"))
			{
				ConfigurationTab::Draw();
				ImGui::EndTabItem();
			}
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