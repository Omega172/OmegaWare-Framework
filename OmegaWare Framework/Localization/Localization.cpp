#include "pch.h"
#include "Localization.h"
#include "Locales/English.h"
#include "Locales/German.h"

Localization::Localization()
{
	LoadLocale(English);
	LoadLocale(German);

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
	for (LocaleData Entry : Cheat::CurrentLocale.Locales)
	{
		if (Entry.Key == KeyHash)
			return Entry.Value;
	}

	return "NOT_FOUND";
}

void Localization::LoadLocale(LocalizationData Locale) { Cheat::Locales.push_back(Locale); }

bool Localization::SetLocale(std::string LocaleCode)
{
	size_t LocaleCodeHash = HASH(LocaleCode);
	for (LocalizationData Locale : Cheat::Locales)
	{
		if (Locale.LocaleCode == LocaleCodeHash)
		{
			Cheat::CurrentLocale = Locale;
			return true;
		}
	}

	return false;
}

bool Localization::SetLocale(size_t LocaleCodeHash)
{
	for (LocalizationData Locale : Cheat::Locales)
	{
		if (Locale.LocaleCode == LocaleCodeHash)
		{
			Cheat::CurrentLocale = Locale;
			return true;
		}
	}

	return false;
}

std::vector<LocalizationData> Localization::GetLocales() { return Cheat::Locales; }

bool Localization::AddToLocale(std::string LocaleCode, std::string Key, std::string Value)
{
	for (LocalizationData& Locale : Cheat::Locales)
	{
		size_t LocaleCodeHash = HASH(LocaleCode);
		if (Locale.LocaleCode == LocaleCodeHash)
		{
			Locale.Locales.push_back({ HASH(Key), Value });
			return true;
		}
	}

	return false;
}

bool Localization::UpdateLocale()
{
	Cheat::CurrentLocale.LocaleCode;

	for (LocalizationData Locale : Cheat::Locales)
	{
		if (Locale.LocaleCode == Cheat::CurrentLocale.LocaleCode)
		{
			Cheat::CurrentLocale = Locale;
			return true;
		}
	}

	return false;
}