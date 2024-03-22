#include "pch.h"
#include "Localization.h"
#include "Locales/English.h"
#include "Locales/German.h"
#include "Locales/Polish.h"

Localization::Localization()
{
	LoadLocale(English);
	LoadLocale(German);
	LoadLocale(Polish);

	if (!SetLocale("ENG"))
	{
		LogErrorHere("Failed to set default locale!");
		return;
	}

	bInitialized = true;
};

bool Localization::IsInitialized() { return bInitialized; }

std::string Localization::Get(std::string Key)
{
	return Get(CRC64::hash(Key));
}

std::string Localization::Get(size_t ullKeyHash)
{
	for (LocaleData Entry : Cheat::CurrentLocale.Locales)
	{
		if (Entry.Key == ullKeyHash)
			return Entry.Value;
	}

	// If not foun in the current locale, try to find it in the english (the default locale)
	constexpr size_t ullENGHash = "ENG"_hash;
	for (LocalizationData Locale : Cheat::Locales)
	{
		if (Locale.LocaleCode != ullENGHash)
			continue;

		for (LocaleData Entry : Locale.Locales)
		{
			if (Entry.Key == ullKeyHash)
				return Entry.Value;
		}
	}

	return "NOT_FOUND"; // If not found in any locale, return NOT_FOUND
}

std::string Localization::StaticGet(size_t ullKeyHash)
{
	return Cheat::localization->Get(ullKeyHash);
}

void Localization::LoadLocale(LocalizationData Locale) { Cheat::Locales.push_back(Locale); }

bool Localization::SetLocale(std::string LocaleCode)
{
	size_t ullLocaleCodeHash = CRC64::hash(LocaleCode);
	for (LocalizationData Locale : Cheat::Locales)
	{
		if (Locale.LocaleCode != ullLocaleCodeHash)
			continue;

		Cheat::CurrentLocale = Locale;

		CurrentFont = *Locale.Font;
		CurrentFontESP = *Locale.FontESP;

		return true;
	}

	return false;
}

bool Localization::SetLocale(size_t ullLocaleCodeHash)
{
	for (LocalizationData Locale : Cheat::Locales)
	{
		if (Locale.LocaleCode != ullLocaleCodeHash)
			continue;

		Cheat::CurrentLocale = Locale;

		CurrentFont = *Locale.Font;
		CurrentFontESP = *Locale.FontESP;

		return true;
	}

	return false;
}

std::vector<LocalizationData> Localization::GetLocales() { return Cheat::Locales; }

bool Localization::AddToLocale(std::string LocaleCode, std::string Key, std::string Value)
{
	size_t ullLocaleCodeHash = CRC64::hash(LocaleCode);
	for (LocalizationData& Locale : Cheat::Locales)
	{
		if (Locale.LocaleCode == ullLocaleCodeHash)
		{
			Locale.Locales.push_back({ CRC64::hash(Key), Value });
			return true;
		}
	}

	return false;
}

bool Localization::AddToLocale(std::string LocaleCode, std::vector<LocaleData> arrLocaleData)
{
	size_t ullLocaleCodeHash = CRC64::hash(LocaleCode);
	for (LocalizationData& Locale : Cheat::Locales)
	{
		if (Locale.LocaleCode == ullLocaleCodeHash)
		{
			for (LocaleData LocaleData : arrLocaleData)
				Locale.Locales.push_back(LocaleData);

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