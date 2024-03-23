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
			guiCheat->SetCallback([]() { return ImVec2(ImGui::GetContentRegionAvail().x / 2, ImGui::GetContentRegionAvail().y / 2); });

			guiCheat->AddElement(guiCheatText.get());
			guiCheat->AddElement(guiCheatSpacing1.get());
			guiCheat->AddElement(guiUnloadButton.get());
			guiUnloadButton->SetCallback([]() {
				Cheat::bShouldRun = false;
			});
			guiCheat->AddElement(guiConsoleVisibility.get());
			guiConsoleVisibility->SetCallback([]() {
				Cheat::console->ToggleVisibility();

				guiConsoleVisibility->SetName(Cheat::console->GetVisibility() ? "CONSOLE_HIDE"_hash : "CONSOLE_SHOW"_hash);
			});
			guiCheat->AddElement(guiLocalization.get());
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
			guiCheat->AddElement(guiWatermark.get());
			guiCheat->AddElement(guiWatermarkFPS.get());
			guiCheat->AddElement(guiSaveConfig.get());
			guiSaveConfig->SetCallback([]() {
				Cheat::config->SaveConfig();
			});
			guiCheat->AddElement(guiLoadConfig.get());
			guiLoadConfig->SetCallback([]() {
				Cheat::config->LoadConfig();
			});
		});


		if (!guiCheat->HasParent())
		{
			Cheat::menu->AddElement(guiCheat.get());
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