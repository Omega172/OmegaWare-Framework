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
			ImGui::Text("Cheat");
			ImGui::Spacing();

			if (ImGui::Button("Unload"))
				Cheat::bShouldRun = false;
			ImGui::SameLine();
			if (ImGui::Button(Cheat::console->GetVisibility() ? "Hide Console" : "Show Console"))
				Cheat::console->ToggleVisibility();

			//ImGui::Checkbox("Extra Debug Info", &bExtraDebug);

			ImGui::Checkbox("Watermark", &Cheat::bWatermark);
			if (Cheat::bWatermark)
				ImGui::Checkbox("Watermark FPS", &Cheat::bWatermarkFPS);

			if (ImGui::Button("Save Config"))
				Cheat::config->SaveConfig();

			ImGui::SameLine();

			if (ImGui::Button("Load Config"))
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