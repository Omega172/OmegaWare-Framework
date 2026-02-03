#include "pch.h"
#include "Watermark.hpp"

void GUI::Render()
{
	if (!Framework::bInitalized)
		return;

	if (ImGui::IsKeyPressed(Framework::keyMenuKey) || ImGui::IsKeyPressed(ImGuiKey_GamepadStart))
	{
		bMenuOpen = !bMenuOpen;
		ImGui::GetIO().MouseDrawCursor = GUI::bMenuOpen;

		if (ImGui::GetIO().MouseDrawCursor)
			SetCursor(NULL);
	}

	if (ImGui::IsKeyPressed(Framework::keyConsoleKey))
		Framework::console->ToggleVisibility();

	if (ImGui::IsKeyPressed(Framework::keyUnloadKey1) || ImGui::IsKeyPressed(Framework::keyUnloadKey2))
		Framework::bShouldRun = false;

	for (auto& pFeature : Framework::g_vecFeatures)
		pFeature->HandleInput();

	/*
	if (GuiWatermark->GetValue())
		ShowWatermark(GuiWatermarkFPS->GetValue(), Framework::Title.c_str(), ImVec4(255, 255, 255, 255), ImVec4(255, 255, 255, 0));
	*/

	if (bMenuOpen)
	{
		static std::once_flag onceflag;
		std::call_once(onceflag, []() {
			/*
			GuiCheat->SetCallback([]() {
				ImGuiContext* pContext = ImGui::GetCurrentContext();

				ImVec2 vec2Size = (Framework::menu->m_stStyle.vec2Size / ImVec2{ 3.f, 2.f }) - pContext->Style.ItemSpacing;
				ImVec2 vec2MaxSize = ImGui::GetContentRegionAvail();

				if (vec2Size.x > vec2MaxSize.x)
					vec2Size.x = vec2MaxSize.x;

				if (vec2Size.y > vec2MaxSize.y)
					vec2Size.y = vec2MaxSize.y;

				return vec2Size;
			});

			GuiCheat->AddElement(GuiCheatSpacing1.get());
			GuiCheat->AddElement(GuiUnloadButton.get());
			GuiUnloadButton->SetCallback([]() {
				Framework::bShouldRun = false;
			});
			GuiCheat->AddElement(GuiConsoleVisibility.get());
			GuiConsoleVisibility->SetCallback([]() {
				Framework::console->ToggleVisibility();

				GuiConsoleVisibility->SetName(Framework::console->GetVisibility() ? "CONSOLE_HIDE"Hashed : "CONSOLE_SHOW"Hashed);
			});
			GuiCheat->AddElement(GuiLocalization.get());
			GuiLocalization->SetCallback([]() {
				std::vector<Locale_t> vecLocales = Localization::GetLocales();
				for (size_t i = 0; i < vecLocales.size(); ++i)
				{
					bool bSelected = Localization::GetCurrentLocaleIndex() == i;
					Locale_t stLocale = vecLocales.at(i);
					if (ImGui::Selectable(stLocale.sKey.c_str(), bSelected))
					{
						Localization::SetLocale(stLocale.ullKeyHash);
						GuiLocalization->SetPreviewLabel(stLocale.sKey.c_str());
					}

					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}
			});
			GuiCheat->AddElement(GuiWatermark.get());
			GuiCheat->AddElement(GuiWatermarkFPS.get());
			GuiCheat->AddElement(GuiSaveConfig.get());
			GuiSaveConfig->SetCallback([]() {
				Framework::config->SaveConfig();
			});
			GuiCheat->AddElement(GuiLoadConfig.get());
			GuiLoadConfig->SetCallback([]() {
				Framework::config->LoadConfig();
			});
			*/

			GuiSidebar->SetPushVarsCallback([]() {
				ImGuiStyle& imStyle = ImGui::GetStyle();
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, imStyle.ChildRounding);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, imStyle.Colors[ImGuiCol_Header]);
			});

			GuiSidebar->SetPopVarsCallback([]() {
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			});

			GuiSidebar->AddElement(GuiMiscSeperator.get());
			GuiSidebar->AddElement(GuiDeveloper.get());
			GuiSidebar->AddElement(GuiStyle.get());
			GuiSidebar->AddElement(GuiSettings.get());
			GuiSidebar->AddElement(GuiConfig.get());

			GuiHeaderGroup->AddElement(GuiBody.get());
		});

		/*
		GuiWatermarkFPS->SetVisible(GuiWatermark->GetValue());

		if (!GuiCheat->HasParent())
		{
			Framework::menu->AddElement(GuiCheat.get());
		}
		*/

		if (!GuiSidebar->HasParent()) {
			Framework::menu->AddElement(GuiSidebar.get());
			Framework::menu->AddElement(GuiHeaderGroup.get());
		}

		for (auto& pFeature : Framework::g_vecFeatures)
			pFeature->HandleMenu();

		Framework::menu->Render();
	}

	//
	//	Other Render Stuff
	//

	for (auto& pFeature : Framework::g_vecFeatures)
		pFeature->Render();

	// Gui init shit 
	if (Framework::menu->HasChildren()) // We have to wait till the menu has children to do the init
	{
		std::call_once(LoadFlag, []() {
			// The menu is opened on load so spawn the mouse
			ImGui::GetIO().MouseDrawCursor = GUI::bMenuOpen;
			if (ImGui::GetIO().MouseDrawCursor)
				SetCursor(NULL);

			// Set localization preview to the loaded locale
			//GuiLocalization->SetPreviewLabel((Localization::GetInstance())->GetLocales()[(Localization::GetInstance())->GetCurrentLocaleIndex()].sKey.c_str());

			// Load the config
			Framework::config->LoadConfig();
		});
	}
  
	//
	// End Other Render Stuff
	//
}