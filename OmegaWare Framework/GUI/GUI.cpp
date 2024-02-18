#include "pch.h"
#include "Watermark.h"

void GUI::Render()
{
	if (Cheat::bWatermark)
		showWatermark(Cheat::bWatermarkFPS, Cheat::Title.c_str(), ImVec4(255, 255, 255, 255), ImVec4(255, 255, 255, 0));

	if (bMenuOpen)
	{
		ImGui::SetNextWindowSize(ImVec2(WIDTH, HEIGHT));
		ImGui::Begin(Cheat::Title.c_str(), NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse);

		ImGui::BeginChild("Cheat", ImVec2(ImGui::GetContentRegionAvail().x / 3, ImGui::GetContentRegionAvail().y), true);
		{
			ImGui::Text(Cheat::localization->Get("CHEAT").c_str());
			ImGui::Spacing();

			if (ImGui::Button(Cheat::localization->Get("UNLOAD_BTN").c_str()))
				Cheat::bShouldRun = false;
			ImGui::SameLine();
			if (ImGui::Button(Cheat::console->GetVisibility() ? Cheat::localization->Get("CONSOLE_HIDE").c_str() : Cheat::localization->Get("CONSOLE_SHOW").c_str()))
				Cheat::console->ToggleVisibility();

			//ImGui::Checkbox("Extra Debug Info", &bExtraDebug);

			ImGui::Checkbox(Cheat::localization->Get("WATER_MARK").c_str(), &Cheat::bWatermark);
			if (Cheat::bWatermark)
				ImGui::Checkbox(Cheat::localization->Get("WATER_MARK_FPS").c_str(), &Cheat::bWatermarkFPS);

			if (ImGui::Button(Cheat::localization->Get("SAVE_CONFIG").c_str()))
				Cheat::config->SaveConfig();

			ImGui::SameLine();

			if (ImGui::Button(Cheat::localization->Get("LOAD_CONFIG").c_str()))
				Cheat::config->LoadConfig();
		}
		ImGui::EndChild();

		for (size_t i = 0; i < Features.size(); i++)
		{
			Features[i]->DrawMenuItems();
		}

		ImGui::End();
	}

	//
	//	Other Render Stuff
	//

#if FRAMEWORK_UNREAL
	auto pUnreal = Cheat::unreal.get();
	pUnreal->RefreshActorList();
#endif

	for (size_t i = 0; i < Features.size(); i++)
	{
		Features[i]->Render();
	}

	//
	// End Other Render Stuff
	//
}