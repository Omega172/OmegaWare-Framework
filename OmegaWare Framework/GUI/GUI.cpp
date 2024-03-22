#include "pch.h"
#include "Watermark.h"


void GUI::Render()
{
	if (!Cheat::bInitalized)
		return;

	if (ImGui::IsKeyPressed(Cheat::keyMenuKey) || ImGui::IsKeyPressed(ImGuiKey_GamepadStart))
	{
		bMenuOpen = !bMenuOpen;
		ImGui::GetIO().MouseDrawCursor = GUI::bMenuOpen;

		if (ImGui::GetIO().MouseDrawCursor)
			SetCursor(NULL);
	}

	if (ImGui::IsKeyPressed(Cheat::keyConsoleKey))
		Cheat::console->ToggleVisibility();

	if (ImGui::IsKeyPressed(Cheat::keyUnloadKey1) || ImGui::IsKeyPressed(Cheat::keyUnloadKey2))
		Cheat::bShouldRun = false;

	for (size_t i = 0; i < Features.size(); i++)
	{
		Features[i]->HandleKeys();
	}

	if (guiWatermark->GetValue())
		showWatermark(guiWatermarkFPS->GetValue(), Cheat::Title.c_str(), ImVec4(255, 255, 255, 255), ImVec4(255, 255, 255, 0));

	if (bMenuOpen)
	{
		static std::once_flag onceflag;

		std::call_once(onceflag, []() {
			guiCheat->AddElement(static_cast<ElementBase*>(guiCheatText.get()), "CHEAT", {});
			guiCheat->AddElement(static_cast<ElementBase*>(guiCheatSpacing1.get()), "SPACING_1", {});
			guiCheat->AddElement(static_cast<ElementBase*>(guiUnloadButton.get()), "UNLOAD_BTN", {});
			guiUnloadButton->SetCallback([]() {
				Cheat::bShouldRun = false;
			});
			guiCheat->AddElement(static_cast<ElementBase*>(guiConsoleVisibility.get()), "CONSOLE_VISIBILITY", {
				.bSameLine = true,
			});
			guiConsoleVisibility->SetCallback([]() {
				Cheat::console->ToggleVisibility();
			});
			guiCheat->AddElement(static_cast<ElementBase*>(guiLocalization.get()), "LANGUAGE", {});
			guiLocalization->SetCallback([]() {
				for (LocalizationData Locale : Cheat::Locales)
				{
					bool bSelected = Cheat::CurrentLocale.LocaleCode == Locale.LocaleCode;
					if (ImGui::Selectable(Locale.Name.c_str(), bSelected))
					{
						Cheat::CurrentLocale = Locale;
						Cheat::localization->SetLocale(Locale.LocaleCode);
					}
					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}
			});
			guiCheat->AddElement(static_cast<ElementBase*>(guiWatermark.get()), "WATER_MARK", {});
			guiCheat->AddElement(static_cast<ElementBase*>(guiWatermarkFPS.get()), "WATER_MARK_FPS", {});
			guiCheat->AddElement(static_cast<ElementBase*>(guiSaveConfig.get()), "SAVE_CONFIG", {});
			guiSaveConfig->SetCallback([]() {
				Cheat::config->SaveConfig();
			});
			guiCheat->AddElement(static_cast<ElementBase*>(guiLoadConfig.get()), "LOAD_CONFIG", {
				.bSameLine = true,
			});
			guiLoadConfig->SetCallback([]() {
				Cheat::config->LoadConfig();
			});
		});


		if (!guiCheat->HasParent())
		{
			Cheat::menu->AddElement(static_cast<ElementBase*>(guiCheat.get()), "Cheat", {
				.iFlags = ImGuiChildFlags_Border,
			});
		}

		for (size_t i = 0; i < Features.size(); i++)
		{
			Features[i]->HandleMenu();
		}

		Cheat::menu->Render();
	}

	//
	//	Other Render Stuff
	//

#if FRAMEWORK_UNREAL
	Cheat::unreal->ActorLock.lock();
	for (size_t i = 0; i < Features.size(); i++)
	{
		Features[i]->Render();
	}
	Cheat::unreal->ActorLock.unlock();
#endif

	//
	// End Other Render Stuff
	//
}