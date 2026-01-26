#include "pch.h"
#include "Core/Common/GlobalState.h"
#include "Core/UserInterface/Tabs/ConfigurationTab.h"
#include "External/imgui/imgui.h"

void ConfigurationTab::Draw()
{
	ImGui::TextDisabled("USER INTERFACE");

	ImGui::BulletText("Toggle Menu:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "CTRL + A");

	ImGui::Spacing();

	ImGui::BulletText("Emergency Reset:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.2f, 0.7f, 1.0f, 1.0f), "CTRL + R");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Recover the window if it goes off-screen.");
	}

	ImGui::Spacing();

	bool blockMouse = g_pState->freezeMouse.load();
	if (ImGui::Checkbox("Freeze Mouse Input", &blockMouse))
	{
		g_pState->freezeMouse.store(blockMouse);
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip(
			"Prevents the game camera from moving while the Control Panel is open.\n"
			"Useful for precise UI interaction during Theater mode."
		);
	}

	ImGui::Spacing();

	static float menuAlpha = 1.0f;
	if (ImGui::SliderFloat("Menu Opacity", &menuAlpha, 0.2f, 1.0f, "%.2f"))
	{
		if (menuAlpha < 0.2f) menuAlpha = 0.2f;

		ImGui::GetStyle().Alpha = menuAlpha;
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Adjust the overall transparency. The minimum is 0.20 to avoid invisibility.");
	}

	ImGui::Spacing();

	ImGui::TextDisabled("FILESYSTEM PATHS");

	{
		std::lock_guard lock(g_pState->configMutex);

		auto PathField = [](const char* label, const std::string& path) {
			ImGui::Text(label);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

			ImGui::InputText(std::string("##").append(label).c_str(),
				(char*)path.c_str(), path.size(), ImGuiInputTextFlags_ReadOnly
			);

			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Right-click to copy path");
				if (ImGui::IsMouseClicked(1))
				{
					ImGui::SetClipboardText(path.c_str());
				}
			}

			ImGui::PopStyleColor();
			ImGui::Spacing();
		};

		PathField("Base Directory", g_pState->baseDirectory);
		PathField("Logger Path", g_pState->loggerPath);
		PathField("Film Path", g_pState->filmPath);
	}
}