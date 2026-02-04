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
		static std::once_flag onceflag;
		std::call_once(onceflag, []() {
			// Setup Developer Page
			GuiDeveloperBodyGroup->SetCallback([]() {
				GuiUnloadButton->SetCallback([]() {
					Framework::bShouldRun = false;
				});
				GuiUnloadButton->Render();

				GuiConsoleVisibility->SetCallback([]() {
					Framework::console->ToggleVisibility();
					GuiConsoleVisibility->SetName(Framework::console->GetVisibility() ? "CONSOLE_HIDE"Hashed : "CONSOLE_SHOW"Hashed);
				});
				GuiConsoleVisibility->Render();

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
				GuiLocalization->Render();
			});

			// Setup Config Page
			GuiConfigBodyGroup->SetCallback([]() {
				GuiSaveConfig->SetCallback([]() {
					Framework::config->SaveConfig();
				});
				GuiSaveConfig->Render();

				GuiLoadConfig->SetCallback([]() {
					Framework::config->LoadConfig();
				});
				GuiLoadConfig->Render();
			});

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
			GuiSidebar->AddElement(GuiDeveloper.get());
			GuiSidebar->AddElement(GuiConfig.get());

			// Set the default page (optional - defaults to 0 if not set)
			ElementBase::SetDefaultPage(GuiConfig->GetPageId());

			auto pHeaderGroup = static_cast<HeaderGroup*>(GuiHeaderGroup.get());
			if (pHeaderGroup)
			{
				pHeaderGroup->AddHeaders(GuiDeveloper->GetPageId(), { "DEVELOPER_MAIN"Hashed });
				pHeaderGroup->AddHeaders(GuiConfig->GetPageId(), { "CONFIG_MAIN"Hashed });
			}

			// Update page IDs for body groups
			GuiDeveloperBodyGroup->SetPageId(GuiDeveloper->GetPageId());
			GuiConfigBodyGroup->SetPageId(GuiConfig->GetPageId());

			GuiHeaderGroup->AddElement(GuiBody.get());
			GuiBody->AddElement(GuiDeveloperBodyGroup.get());
			GuiBody->AddElement(GuiConfigBodyGroup.get());
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