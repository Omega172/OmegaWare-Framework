#include "pch.h"
#include "Localization.h""
#include "Locales/English.h"

Localization::Localization()
{
	LoadLocale(English);

	if (!SetLocale("ENG"))
	{
		Utils::LogError(Utils::GetLocation(CurrentLoc), "Failed to set default locale");
		return;
	}

	bInitialized = true;
};

bool Localization::IsInitialized() { return bInitialized; }

std::string Localization::Get(std::string Key)
{
	for (LocaleData Entry : Cheat::CurrentLocale.Locals)
	{
		if (Entry.Key == Key)
			return Entry.Value;
	}

	return "NOT_FOUND";
}

void Localization::LoadLocale(LocalizationData Locale) { Cheat::Locales.push_back(Locale); }

bool Localization::SetLocale(std::string LocalCode)
{
	for (LocalizationData Locale : Cheat::Locales)
	{
		if (Locale.LocalCode == LocalCode)
		{
			Utils::LogDebug(Utils::GetLocation(CurrentLoc), "Set locale to " + LocalCode);
			Cheat::CurrentLocale = Locale;
			return true;
		}
	}

	return false;
}

std::vector<LocalizationData> Localization::GetLocales() { return Cheat::Locales; }

bool Localization::AddToLocale(std::string LocalCode, std::string Key, std::string Value)
{
	for (LocalizationData& Locale : Cheat::Locales)
	{
		if (Locale.LocalCode == LocalCode)
		{
			Locale.Locals.push_back({ Key, Value });
			return true;
		}
	}

	return false;
}