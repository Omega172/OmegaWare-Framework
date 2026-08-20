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
			// SetupStyle() has already run by now (it happens once on the WndProc thread
			// before the render loop starts) - snapshot it as the un-scaled base for UI_SCALE.
			BaseStyle = ImGui::GetStyle();

			GuiColorWindowBg->SetValue(BaseStyle.Colors[ImGuiCol_WindowBg]);
			GuiColorChildBg->SetValue(BaseStyle.Colors[ImGuiCol_ChildBg]);
			GuiColorChrome->SetValue(BaseStyle.Colors[ImGuiCol_ChildBg]);
			GuiColorHeader->SetValue(BaseStyle.Colors[ImGuiCol_Header]);
			GuiColorBorder->SetValue(BaseStyle.Colors[ImGuiCol_Border]);
			GuiColorText->SetValue(BaseStyle.Colors[ImGuiCol_Text]);
			GuiColorAccent->SetValue(BaseStyle.Colors[ImGuiCol_SliderGrab]);
			GuiColorButton->SetValue(BaseStyle.Colors[ImGuiCol_Button]);

			GuiSidebar->SetPushVarsCallback([]() {
				ImGuiStyle& imStyle = ImGui::GetStyle();
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, imStyle.ChildRounding);
			});

			GuiSidebar->SetPopVarsCallback([]() {
				ImGui::PopStyleVar();
			});

			GuiSidebar->AddElement(GuiFeatureSeperator.get());
			GuiSidebar->AddElement(GuiMiscSeperator.get());
			GuiSidebar->AddElement(GuiStyle.get());
			GuiSidebar->AddElement(GuiSettings.get());
			GuiSidebar->AddElement(GuiDeveloper.get());

			// Set the default page (optional - defaults to 0 if not set)
			ElementBase::SetDefaultPage(GuiSettings->GetPageId());

			auto pHeaderGroup = static_cast<HeaderGroup*>(GuiHeaderGroup.get());
			if (pHeaderGroup)
			{
				pHeaderGroup->AddHeaders(GuiDeveloper->GetPageId(), { "DEVELOPER_MAIN"Hashed });
				pHeaderGroup->AddHeaders(GuiSettings->GetPageId(), { "SETTINGS_MAIN"Hashed });
				pHeaderGroup->AddHeaders(GuiStyle->GetPageId(), { "STYLE_MAIN"Hashed });
			}

			GuiDeveloperPage->SetPageId(GuiDeveloper->GetPageId());
			GuiConsoleVisibility->SetCallback([]() {
				Framework::console->ToggleVisibility();
				GuiConsoleVisibility->SetName(Framework::console->GetVisibility() ? "CONSOLE_HIDE"Hashed : "CONSOLE_SHOW"Hashed);
			});
			GuiDeveloperPage->AddElement(GuiConsoleVisibility.get());

			GuiOpenLogsFolder->SetCallback([]() {
				auto optPath = Utils::GetLogFilePath();
				if (optPath)
					Utils::OpenFolder(optPath.value());
			});
			GuiDeveloperPage->AddElement(GuiOpenLogsFolder.get());

			GuiSettingsPage->SetPageId(GuiSettings->GetPageId());

			GuiUnloadButton->SetCallback([]() {
				Framework::bShouldRun = false;
			});
			GuiUnloadButton->SetPositionCallback([]() {
				const float flTargetY = ImGui::GetCursorPosY() + ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeight();
				if (flTargetY > ImGui::GetCursorPosY())
					ImGui::SetCursorPosY(flTargetY);
			});

			auto ComputeHalfComboWidth = []() {
				const ImGuiStyle& comboStyle = ImGui::GetStyle();
				return (ImGui::GetWindowWidth() - comboStyle.WindowPadding.x * 2.0f - comboStyle.ItemSpacing.x) / 2.0f;
			};
			GuiUIScale->SetWidthCallback(ComputeHalfComboWidth);
			GuiLocalization->SetWidthCallback(ComputeHalfComboWidth);

			GuiSettingsLeftGroup->SetCallback([]() {
				float fGroupWidth = (ImGui::GetWindowWidth() - 10.0f - 10.0f * 2) / 2;
				return ImVec2(fGroupWidth, 0.f);
			});
			GuiConfigGroup->SetCallback([]() {
				float fGroupWidth = (ImGui::GetWindowWidth() - 10.0f - 10.0f * 2) / 2;
				return ImVec2(fGroupWidth, 0.f);
			});

			ConfigManager::Callbacks stConfigCallbacks{};

			stConfigCallbacks.GetCurrentName = []() {
				return Framework::config->GetCurrentConfigName();
			};
			stConfigCallbacks.GetDefaultName = []() {
				return Framework::config->GetDefaultConfigName();
			};
			stConfigCallbacks.ListConfigs = []() {
				return Framework::config->ListConfigs();
			};
			stConfigCallbacks.Load = [](const std::string& sName) {
				Framework::config->LoadConfigNamed(*Framework::menu, sName);
			};
			stConfigCallbacks.Save = []() {
				Framework::config->SaveConfig(*Framework::menu);
			};
			stConfigCallbacks.Reload = []() {
				Framework::config->LoadConfig(*Framework::menu);
			};
			stConfigCallbacks.CreateNew = [](const std::string& sName) {
				Framework::config->WriteConfigJson(jsonConfigDefaults, sName);
			};
			stConfigCallbacks.Rename = [](const std::string& sOldName, const std::string& sNewName) {
				Framework::config->RenameConfig(sOldName, sNewName);
			};
			stConfigCallbacks.Delete = [](const std::string& sName) {
				const bool bWasCurrent = (sName == Framework::config->GetCurrentConfigName());
				Framework::config->DeleteConfig(sName);

				if (bWasCurrent)
				{
					std::vector<std::string> vecRemaining = Framework::config->ListConfigs();
					if (!vecRemaining.empty())
						Framework::config->LoadConfigNamed(*Framework::menu, vecRemaining.front());
				}
			};
			stConfigCallbacks.SetDefault = [](const std::string& sName) {
				Framework::config->SetDefaultConfig(sName);
			};
			stConfigCallbacks.EnsureDefaultValid = []() {
				Framework::config->EnsureDefaultValid();
			};
			stConfigCallbacks.OpenFolder = []() {
				Utils::OpenFolder(Framework::config->GetConfigDir());
			};

			GuiConfigManager->SetCallbacks(stConfigCallbacks);

			GuiConfigGroup->AddElement(GuiConfigManager.get());

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
			for (float flScale = kUIScaleMin; flScale <= kUIScaleMax + 0.001f; flScale += kUIScaleStep)
				GuiUIScale->AddOption(std::format("{:.2f}x", flScale));

			GuiSettingsLeftGroup->AddElement(GuiUIScale.get());
			GuiSettingsLeftGroup->AddElement(GuiLocalization.get());
			GuiSettingsLeftGroup->AddElement(GuiUnloadButton.get());

			GuiSettingsPage->AddElement(GuiSettingsLeftGroup.get());
			GuiSettingsPage->AddElement(GuiConfigGroup.get());

			GuiStylePage->SetPageId(GuiStyle->GetPageId());

			GuiResetStyle->SetCallback([]() {
				GuiColorWindowBg->SetValue(BaseStyle.Colors[ImGuiCol_WindowBg]);
				GuiColorChildBg->SetValue(BaseStyle.Colors[ImGuiCol_ChildBg]);
				GuiColorChrome->SetValue(BaseStyle.Colors[ImGuiCol_ChildBg]);
				GuiColorHeader->SetValue(BaseStyle.Colors[ImGuiCol_Header]);
				GuiColorBorder->SetValue(BaseStyle.Colors[ImGuiCol_Border]);
				GuiColorText->SetValue(BaseStyle.Colors[ImGuiCol_Text]);
				GuiColorAccent->SetValue(BaseStyle.Colors[ImGuiCol_SliderGrab]);
				GuiColorButton->SetValue(BaseStyle.Colors[ImGuiCol_Button]);
			});
			GuiStylePage->AddElement(GuiResetStyle.get());

			GuiColorGroup->AddElement(GuiColorWindowBg.get());
			GuiColorGroup->AddElement(GuiColorChildBg.get());
			GuiColorGroup->AddElement(GuiColorChrome.get());
			GuiColorGroup->AddElement(GuiColorHeader.get());
			GuiColorGroup->AddElement(GuiColorBorder.get());
			GuiColorGroup->AddElement(GuiColorText.get());
			GuiColorGroup->AddElement(GuiColorAccent.get());
			GuiColorGroup->AddElement(GuiColorButton.get());
			GuiStylePage->AddElement(GuiColorGroup.get());

			GuiPreviewCombo->AddOption("Option 1");
			GuiPreviewCombo->AddOption("Option 2");
			GuiPreviewCombo->AddOption("Option 3");

			GuiPreviewGroup->AddElement(GuiPreviewButton.get());
			GuiPreviewGroup->AddElement(GuiPreviewAccentButton.get());
			GuiPreviewGroup->AddElement(GuiPreviewToggle.get());
			GuiPreviewGroup->AddElement(GuiPreviewCheckbox.get());
			GuiPreviewGroup->AddElement(GuiPreviewSlider.get());
			GuiPreviewGroup->AddElement(GuiPreviewCombo.get());
			GuiPreviewGroup->AddElement(GuiPreviewText.get());

			GuiPreviewTable->AddColumn("PREVIEW_TABLE_COL_A"Hashed);
			GuiPreviewTable->AddColumn("PREVIEW_TABLE_COL_B"Hashed);
			GuiPreviewTable->AddRow({ "Row 1", "1" });
			GuiPreviewTable->AddRow({ "Row 2", "2" });
			GuiPreviewTable->SetSaveToConfig(false); // just preview data, don't persist it
			GuiPreviewGroup->AddElement(GuiPreviewTable.get());
			GuiStylePage->AddElement(GuiPreviewGroup.get());

			GuiHeaderGroup->AddElement(GuiBody.get());
			GuiBody->AddElement(GuiDeveloperPage.get());
			GuiBody->AddElement(GuiSettingsPage.get());
			GuiBody->AddElement(GuiStylePage.get());
		});

		if (!GuiSidebar->HasParent()) {
			Framework::menu->AddElement(GuiSidebar.get());
			Framework::menu->AddElement(GuiHeaderGroup.get());
		}

		{
			const ImGuiStyle& sidebarStyle = ImGui::GetStyle();
			float flNaturalWidth = kMinSidebarWidth;
			for (ElementBase* pChild : GuiSidebar->GetChildren())
			{
				if (!pChild)
					continue;

				flNaturalWidth = ImMax(flNaturalWidth, pChild->GetNaturalSize().x + sidebarStyle.WindowPadding.x * 2.f);
			}

			kSidebarWidth = flNaturalWidth;
			GuiSidebar->SetWidth(kSidebarWidth);
		}

		const float flUIScale = kUIScaleMin + kUIScaleStep * static_cast<float>(GuiUIScale->GetSelectedIndex());
		if (flUIScale != fLastAppliedUIScale)
		{
			ImGuiStyle style = BaseStyle;
			style.ScaleAllSizes(flUIScale);
			ImGui::GetStyle() = style;

			bPendingFontRebuild = true;
			flPendingUIScale = flUIScale;

			Framework::menu->RequestResize(GUI::kBaseMenuSize * flUIScale);

			fLastAppliedUIScale = flUIScale;
			g_flUIScale = flUIScale;
		}

		{
			ImGuiStyle& liveStyle = ImGui::GetStyle();

			liveStyle.Colors[ImGuiCol_WindowBg] = GuiColorWindowBg->GetValue();
			liveStyle.Colors[ImGuiCol_PopupBg] = GuiColorWindowBg->GetValue();

			liveStyle.Colors[ImGuiCol_ChildBg] = GuiColorChildBg->GetValue();
			liveStyle.Colors[ImGuiCol_TitleBg] = GuiColorChildBg->GetValue();
			liveStyle.Colors[ImGuiCol_TitleBgActive] = GuiColorChildBg->GetValue();

			// Sidebar/header/footer chrome backdrop - Menu::Render() reads this directly rather
			// than a live ImGuiStyle slot (see vec4ChromeBg's own comment in Elements.hpp).
			vec4ChromeBg = GuiColorChrome->GetValue();

			liveStyle.Colors[ImGuiCol_Text] = GuiColorText->GetValue();
			liveStyle.Colors[ImGuiCol_CheckMark] = GuiColorText->GetValue();
			liveStyle.Colors[ImGuiCol_TextDisabled] = GuiColorText->GetValue();

			const ImVec4 vec4Accent = GuiColorAccent->GetValue();
			liveStyle.Colors[ImGuiCol_SliderGrab] = vec4Accent;
			liveStyle.Colors[ImGuiCol_ScrollbarGrab] = vec4Accent;
			liveStyle.Colors[ImGuiCol_TextSelectedBg] = vec4Accent;

			const ImVec4 vec4AccentActive = ImAdd::ShadeColor(vec4Accent, 0.6f);
			liveStyle.Colors[ImGuiCol_SliderGrabActive] = vec4AccentActive;
			liveStyle.Colors[ImGuiCol_ScrollbarGrabActive] = vec4AccentActive;

			liveStyle.Colors[ImGuiCol_Border] = GuiColorBorder->GetValue();
			liveStyle.Colors[ImGuiCol_Separator] = GuiColorBorder->GetValue();

			const ImVec4 vec4Button = GuiColorButton->GetValue();
			liveStyle.Colors[ImGuiCol_Button] = vec4Button;
			liveStyle.Colors[ImGuiCol_FrameBg] = vec4Button;

			const ImVec4 vec4ButtonHovered = ImAdd::ShadeColor(vec4Button, 0.85f);
			liveStyle.Colors[ImGuiCol_ButtonHovered] = vec4ButtonHovered;
			liveStyle.Colors[ImGuiCol_FrameBgHovered] = vec4ButtonHovered;

			const ImVec4 vec4ButtonActive = ImAdd::ShadeColor(vec4Button, 0.6f);
			liveStyle.Colors[ImGuiCol_ButtonActive] = vec4ButtonActive;
			liveStyle.Colors[ImGuiCol_FrameBgActive] = vec4ButtonActive;

			const ImVec4 vec4Header = GuiColorHeader->GetValue();
			liveStyle.Colors[ImGuiCol_Header] = vec4Header;
			liveStyle.Colors[ImGuiCol_HeaderHovered] = ImVec4(vec4Header.x, vec4Header.y, vec4Header.z, vec4Header.w * 0.7f);
			liveStyle.Colors[ImGuiCol_HeaderActive] = ImVec4(vec4Header.x, vec4Header.y, vec4Header.z, vec4Header.w * 0.5f);
		}

		for (auto& pFeature : Framework::g_vecFeatures)
			pFeature->HandleMenu();

		Framework::menu->Render();
	}

	for (auto& pFeature : Framework::g_vecFeatures)
		pFeature->Render();

	if (Framework::menu->HasChildren()) // We have to wait till the menu has children to do the init
	{
		std::call_once(LoadFlag, []() {
			ImGui::GetIO().MouseDrawCursor = GUI::bMenuOpen;
			if (ImGui::GetIO().MouseDrawCursor)
				SetCursor(NULL);

			GuiLocalization->SetPreviewLabel((Localization::GetInstance())->GetLocales()[(Localization::GetInstance())->GetCurrentLocaleIndex()].sKey.c_str());

			Framework::menu->ConfigSave(jsonConfigDefaults);

			Framework::config->LoadConfig(*Framework::menu);
		});
	}
}
