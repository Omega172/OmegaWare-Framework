#include "Features/Feature.hpp"
#include "Core/Log.hpp"

#include "OmegaGUI/OmegaGUI.hpp"
#include "OmegaLocalization/OmegaLocalization.hpp"
#include "OmegaRenderHook/Core/Guarded.hpp"

#include <array>
#include <format>
#include <vector>

namespace OmegaWare
{
	struct stFeatureAccess
	{
		static std::vector<clFeature*>& Registry()
		{
			static std::vector<clFeature*> s_aFeatures;
			return s_aFeatures;
		}

		template <typename F>
		static bool Invoke(clFeature& rFeature, std::string_view strStage, F&& fnCall)
		{
			OmegaRenderHook::stFault stFault;
			auto fnGuarded = [&] { fnCall(); };
			if (OmegaRenderHook::InvokeGuarded(fnGuarded, stFault))
				return true;

			rFeature.m_bActive = false;
			rFeature.m_bFaulted = true;
			if (stFault.bCppException)
				LogError("feature \"{}\" threw a C++ exception in {}, feature disabled", rFeature.GetName(), strStage);
			else
				LogError("feature \"{}\" faulted in {} (code {:#010x} at {}), feature disabled", rFeature.GetName(), strStage, stFault.uCode, stFault.pAddress);
			return false;
		}

		static void Setup(clFeature& rFeature)
		{
			bool bResult = false;
			if (!Invoke(rFeature, "Setup", [&] { bResult = rFeature.Setup(); }))
				return;
			rFeature.m_bSetup = true;
			rFeature.m_bActive = bResult;
			if (bResult)
				LogInfo("feature \"{}\" ready", rFeature.GetName());
			else
				LogWarning("feature \"{}\" setup returned false, feature disabled", rFeature.GetName());
		}

		static void Destroy(clFeature& rFeature)
		{
			rFeature.m_bActive = false;
			if (!rFeature.m_bSetup)
				return;
			rFeature.m_bSetup = false;
			Invoke(rFeature, "Destroy", [&] { rFeature.Destroy(); });
		}
	};

	clFeature::clFeature()
	{
		stFeatureAccess::Registry().push_back(this);
	}

	std::string clFeature::GetDisplayName() const
	{
		return std::string(OmegaLocalization::Get(std::format("feature.{}", GetName())));
	}

	namespace Features
	{
		std::span<clFeature* const> All()
		{
			return stFeatureAccess::Registry();
		}

		void SetupAll()
		{
			for (clFeature* pFeature : stFeatureAccess::Registry())
				stFeatureAccess::Setup(*pFeature);
		}

		void Setup(clFeature& rFeature)
		{
			stFeatureAccess::Setup(rFeature);
		}

		void Remove(clFeature& rFeature)
		{
			stFeatureAccess::Destroy(rFeature);
			std::erase(stFeatureAccess::Registry(), &rFeature);
		}

		void RunAll()
		{
			for (clFeature* pFeature : stFeatureAccess::Registry())
				if (pFeature->IsActive())
					stFeatureAccess::Invoke(*pFeature, "Run", [&] { pFeature->Run(); });
		}

		void RenderAll()
		{
			for (clFeature* pFeature : stFeatureAccess::Registry())
				if (pFeature->IsActive())
					stFeatureAccess::Invoke(*pFeature, "Render", [&] { pFeature->Render(); });
		}

		std::string DisplayName(const clFeature& rFeature)
		{
			return rFeature.GetDisplayName();
		}

		void DrawPage(clFeature& rFeature)
		{
			const std::string strName = DisplayName(rFeature);
			OmegaGUI::Breadcrumbs(std::array<std::string_view, 2>{ "features", rFeature.GetName() });
			OmegaGUI::Title(strName);
			OmegaGUI::Spacing();
			if (rFeature.IsActive())
				stFeatureAccess::Invoke(rFeature, "DrawMenu", [&] { rFeature.DrawMenu(); });
			else
				OmegaGUI::Alert(OmegaGUI::EToastKind::Error, OmegaLocalization::Get(rFeature.IsFaulted() ? "feature.faulted" : "feature.disabled"));
		}

		void DestroyAll()
		{
			auto& rFeatures = stFeatureAccess::Registry();
			for (auto itFeature = rFeatures.rbegin(); itFeature != rFeatures.rend(); ++itFeature)
				stFeatureAccess::Destroy(**itFeature);
		}
	}
}
