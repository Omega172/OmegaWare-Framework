#include "pch.h"

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

	if (bMenuOpen)
	{
		// One-time GUI setup
		static std::once_flag onceflag;
		std::call_once(onceflag, []() {
			ElementBase::SetDefaultPage(GuiConfigSidebarButton->GetPageId());

			// Setup Sidebar
			GuiSidebar->SetPushVarsCallback([]() {
				ImGuiStyle& imStyle = ImGui::GetStyle();
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, imStyle.ChildRounding);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, imStyle.Colors[ImGuiCol_Header]);
			});

			GuiSidebar->SetPopVarsCallback([]() {
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			});

			GuiSidebar->AddElement(GuiFeatureSeperator.get());
			GuiSidebar->AddElement(GuiMiscSeperator.get());

			GuiSidebar->InsertElementAfter(GuiAllFeaturesSidebarButton.get(), "FEATURE_SEPERATOR");
			GuiSidebar->InsertElementAfter(GuiDeveloperSidebarButton.get(), "MISC_SEPERATOR");
			GuiSidebar->InsertElementAfter(GuiConfigSidebarButton.get(), "DEVELOPER_BUTTON");

			// Header
			GuiHeaderGroup->AddHeaders(GuiAllFeaturesSidebarButton->GetPageId(), { "ALL_FEATURES_MAIN"Hashed });
			GuiHeaderGroup->AddHeaders(GuiDeveloperSidebarButton->GetPageId(), { "DEVELOPER_MAIN"Hashed });
			GuiHeaderGroup->AddHeaders(GuiConfigSidebarButton->GetPageId(), { "CONFIG_MAIN"Hashed });

			GuiHeaderGroup->AddElement(GuiBody.get());

			// Body
			GuiBody->AddElement(GuiAllFeaturesPage.get());
			GuiBody->AddElement(GuiDeveloperPage.get());
			GuiBody->AddElement(GuiConfigPage.get());

			// Pages
			GuiAllFeaturesPage->SetPageId(GuiAllFeaturesSidebarButton->GetPageId());

			GuiDeveloperPage->SetPageId(GuiDeveloperSidebarButton->GetPageId());
			GuiDeveloperPage->AddElement(GuiUnloadButton.get());
			GuiDeveloperPage->AddElement(GuiConsoleVisibility.get());
			GuiDeveloperPage->AddElement(GuiLocalization.get());
			
			GuiConfigPage->SetPageId(GuiConfigSidebarButton->GetPageId());
			GuiConfigPage->AddElement(GuiSaveConfig.get());
			GuiConfigPage->AddElement(GuiLoadConfig.get());

			// Developer Page Elements
			GuiUnloadButton->SetCallback([]() {
				Framework::bShouldRun = false;
			});
			GuiConsoleVisibility->SetCallback([]() {
				Framework::console->ToggleVisibility();
				GuiConsoleVisibility->SetName(Framework::console->GetVisibility() ? "CONSOLE_HIDE"Hashed : "CONSOLE_SHOW"Hashed);
			});
			GuiLocalization->SetCallback([]() {
				std::vector<Locale_t> vecLocales = Localization::GetLocales();
				for (size_t i = 0; i < vecLocales.size(); ++i)
				{
					bool bSelected = Localization::GetCurrentLocaleIndex() == i;
					Locale_t stLocale = vecLocales.at(i);
					if (ImAdd::Selectable(stLocale.sKey.c_str(), bSelected))
					{
						Localization::SetLocale(stLocale.ullKeyHash);
						GuiLocalization->SetPreviewLabel(stLocale.sKey.c_str());
					}

					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}
			});

			// Config Page Elements
			GuiSaveConfig->SetCallback([]() {
				Framework::config->SaveConfig();
			});
			GuiLoadConfig->SetCallback([]() {
				Framework::config->LoadConfig();
			});
		});

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

			GuiLocalization->SetPreviewLabel((Localization::GetInstance())->GetLocales()[(Localization::GetInstance())->GetCurrentLocaleIndex()].sKey.c_str());

			// Load the config
			Framework::config->LoadConfig();
		});
	}
  
	//
	// End Other Render Stuff
	//
}