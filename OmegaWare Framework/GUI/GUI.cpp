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

	if (guiWatermark->GetValue())
		showWatermark(guiWatermarkFPS->GetValue(), Framework::Title.c_str(), ImVec4(255, 255, 255, 255), ImVec4(255, 255, 255, 0));

	if (bMenuOpen)
	{
		static std::once_flag onceflag;

		std::call_once(onceflag, []() {
			guiCheat->SetCallback([]() { 
				ImGuiContext* pContext = ImGui::GetCurrentContext();

				ImVec2 vec2Size = (Framework::menu->m_stStyle.vec2Size / ImVec2{ 3.f, 2.f }) - pContext->Style.ItemSpacing;
				ImVec2 vec2MaxSize = ImGui::GetContentRegionAvail();

				if (vec2Size.x > vec2MaxSize.x)
					vec2Size.x = vec2MaxSize.x;

				if (vec2Size.y > vec2MaxSize.y)
					vec2Size.y = vec2MaxSize.y;

				return vec2Size;
			});

			guiCheat->AddElement(guiCheatText.get());
			guiCheat->AddElement(guiCheatSpacing1.get());
			guiCheat->AddElement(guiUnloadButton.get());
			guiUnloadButton->SetCallback([]() {
				Framework::bShouldRun = false;
			});
			guiCheat->AddElement(guiConsoleVisibility.get());
			guiConsoleVisibility->SetCallback([]() {
				Framework::console->ToggleVisibility();

				guiConsoleVisibility->SetName(Framework::console->GetVisibility() ? "CONSOLE_HIDE"_hash : "CONSOLE_SHOW"_hash);
			});
			guiCheat->AddElement(guiLocalization.get());
			guiLocalization->SetCallback([]() {
				std::vector<Locale_t> vecLocales = Localization::GetLocales();
				for (size_t i = 0; i < vecLocales.size(); ++i)
				{
					bool bSelected = Localization::GetCurrentLocaleIndex() == i;
					Locale_t stLocale = vecLocales.at(i);
					if (ImGui::Selectable(stLocale.sKey.c_str(), bSelected))
					{
						Localization::SetLocale(stLocale.ullKeyHash);
					}

					if (bSelected)
						ImGui::SetItemDefaultFocus();
				}
			});
			guiCheat->AddElement(guiWatermark.get());
			guiCheat->AddElement(guiWatermarkFPS.get());
			guiCheat->AddElement(guiSaveConfig.get());
			guiSaveConfig->SetCallback([]() {
				Framework::config->SaveConfig();
			});
			guiCheat->AddElement(guiLoadConfig.get());
			guiLoadConfig->SetCallback([]() {
				Framework::config->LoadConfig();
			});
		});

		guiWatermarkFPS->SetVisible(guiWatermark->GetValue());

		if (!guiCheat->HasParent())
		{
			Framework::menu->AddElement(guiCheat.get());
		}

		for (size_t i = 0; i < Features.size(); i++)
		{
			Features[i]->HandleMenu();
		}

		Framework::menu->Render();
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