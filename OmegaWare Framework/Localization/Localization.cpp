#include "pch.h"
#include "Localization.h"
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
	size_t KeyHash = HASH(Key);
	for (LocaleData Entry : Cheat::CurrentLocale.Locals)
	{
		if (Entry.Key == KeyHash)
			return Entry.Value;
	}

	return "NOT_FOUND";
}

void Localization::LoadLocale(LocalizationData Locale) { Cheat::Locales.push_back(Locale); }

bool Localization::SetLocale(std::string LocalCode)
{
	size_t LocaleCodeHash = HASH(LocalCode);
	for (LocalizationData Locale : Cheat::Locales)
	{
		if (Locale.LocalCode == LocaleCodeHash)
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
		size_t LocaleCodeHash = HASH(LocalCode);
		if (Locale.LocalCode == LocaleCodeHash)
		{
			Locale.Locals.push_back({ HASH(Key), Value });
			return true;
		}
	}

	return false;
}