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

	if (Cheat::bWatermark)
		showWatermark(Cheat::bWatermarkFPS, Cheat::Title.c_str(), ImVec4(255, 255, 255, 255), ImVec4(255, 255, 255, 0));

	if (bMenuOpen)
	{
		auto child = std::make_unique<Child>(Child("Cheat", []() { return ImVec2(ImGui::GetContentRegionAvail().x / 3, ImGui::GetContentRegionAvail().y / 2); }, ImGuiChildFlags_Border));
		child->AddElement(std::make_unique<Text>(Text(Cheat::localization->Get("CHEAT"))));
		child->AddElement(std::make_unique<Spacing>(Spacing()));
		child->AddElement(std::make_unique<Button>(Button(Cheat::localization->Get("UNLOAD_BTN"), []() {
			Cheat::bShouldRun = false;
		})));
		child->AddElement(std::make_unique<Button>(Button(Cheat::console->GetVisibility() ? Cheat::localization->Get("CONSOLE_HIDE") : Cheat::localization->Get("CONSOLE_SHOW"), []() {
			Cheat::console->ToggleVisibility();
		})), true);
		child->AddElement(std::make_unique<Combo>(Combo(Cheat::localization->Get("LANGUAGE"), Cheat::CurrentLocale.Name, NULL, []() {
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
		})));
		child->AddElement(std::make_unique<Checkbox>(Checkbox(Cheat::localization->Get("WATER_MARK"), &Cheat::bWatermark)));
		if (Cheat::bWatermark)
			child->AddElement(std::make_unique<Checkbox>(Checkbox(Cheat::localization->Get("WATER_MARK_FPS"), &Cheat::bWatermarkFPS)));
		child->AddElement(std::make_unique<Button>(Button(Cheat::localization->Get("SAVE_CONFIG"), []() {
			Cheat::config->SaveConfig();
		})));
		child->AddElement(std::make_unique<Button>(Button(Cheat::localization->Get("LOAD_CONFIG"), []() {
			Cheat::config->LoadConfig();
		})), true);

		Cheat::menu->AddElement(std::move(child));

		for (size_t i = 0; i < Features.size(); i++)
		{
			Features[i]->PopulateMenu();
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