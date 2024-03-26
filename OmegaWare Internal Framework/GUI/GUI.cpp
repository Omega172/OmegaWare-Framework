#include "pch.h"
#include "Watermark.h"

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

	for (size_t i = 0; i < Features.size(); i++)
	{
		Features[i]->HandleKeys();
	}

	if (Framework::bWatermark)
		showWatermark(Framework::bWatermarkFPS, Framework::Title.c_str(), ImVec4(255, 255, 255, 255), ImVec4(255, 255, 255, 0));

	if (bMenuOpen)
	{
		auto child = std::make_unique<Child>("Cheat", []() { return ImVec2(ImGui::GetContentRegionAvail().x / 3, ImGui::GetContentRegionAvail().y / 2); }, ImGuiChildFlags_Border);
		child->AddElement(std::make_unique<Text>(Framework::localization->Get("CHEAT")));
		child->AddElement(std::make_unique<Spacing>());
		child->AddElement(std::make_unique<Button>(Framework::localization->Get("UNLOAD_BTN"), []() {
			Framework::bShouldRun = false;
		}));
		child->AddElement(std::make_unique<Button>(Framework::console->GetVisibility() ? Framework::localization->Get("CONSOLE_HIDE") : Framework::localization->Get("CONSOLE_SHOW"), []() {
			Framework::console->ToggleVisibility();
		}), true);
		child->AddElement(std::make_unique<Combo>(Framework::localization->Get("LANGUAGE"), Framework::CurrentLocale.Name, NULL, []() {
			for (LocalizationData Locale : Framework::Locales)
			{
				bool bSelected = Framework::CurrentLocale.LocaleCode == Locale.LocaleCode;
				if (ImGui::Selectable(Locale.Name.c_str(), bSelected))
				{
					Framework::CurrentLocale = Locale;
					Framework::localization->SetLocale(Locale.LocaleCode);
				}
				if (bSelected)
					ImGui::SetItemDefaultFocus();
			}
		}));
		child->AddElement(std::make_unique<Checkbox>(Framework::localization->Get("WATER_MARK"), &Framework::bWatermark));
		if (Framework::bWatermark)
			child->AddElement(std::make_unique<Checkbox>(Framework::localization->Get("WATER_MARK_FPS"), &Framework::bWatermarkFPS));
		child->AddElement(std::make_unique<Button>(Framework::localization->Get("SAVE_CONFIG"), []() {
			Framework::config->SaveConfig();
		}));
		child->AddElement(std::make_unique<Button>(Framework::localization->Get("LOAD_CONFIG"), []() {
			Framework::config->LoadConfig();
		}), true);

		Framework::menu->AddElement(std::move(child));

		for (size_t i = 0; i < Features.size(); i++)
		{
			Features[i]->PopulateMenu();
		}

		Framework::menu->Render();
	}

	//
	//	Other Render Stuff
	//

#if FRAMEWORK_UNREAL
	Framework::unreal->ActorLock.lock();
	for (size_t i = 0; i < Features.size(); i++)
	{
		Features[i]->Render();
	}
	Framework::unreal->ActorLock.unlock();
#endif

	//
	// End Other Render Stuff
	//
}