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
	for (LocaleData Entry : Framework::CurrentLocale.Locales)
	{
		if (Entry.Key == HASH(Key))
			return Entry.Value;
	}

	// If not foun in the current locale, try to find it in the default locale
	for (LocalizationData Locale : Framework::Locales)
	{
		if (Locale.LocaleCode == HASH("ENG"))
		{
			for (LocaleData Entry : Locale.Locales)
			{
				if (Entry.Key == HASH(Key))
					return Entry.Value;
			}
		}
	}

	return "NOT_FOUND"; // If not found in any locale, return NOT_FOUND
}

void Localization::LoadLocale(LocalizationData Locale) { Framework::Locales.push_back(Locale); }

bool Localization::SetLocale(std::string LocaleCode)
{
	size_t LocaleCodeHash = HASH(LocaleCode);
	for (LocalizationData Locale : Framework::Locales)
	{
		if (Locale.LocaleCode == LocaleCodeHash)
		{
			Framework::CurrentLocale = Locale;

			CurrentFont = *Locale.Font;
			CurrentFontESP = *Locale.FontESP;

			return true;
		}
	}

	return false;
}

bool Localization::SetLocale(size_t LocaleCodeHash)
{
	for (LocalizationData Locale : Framework::Locales)
	{
		if (Locale.LocaleCode == LocaleCodeHash)
		{
			Framework::CurrentLocale = Locale;

			CurrentFont = *Locale.Font;
			CurrentFontESP = *Locale.FontESP;

			return true;
		}
	}

	return false;
}

std::vector<LocalizationData> Localization::GetLocales() { return Framework::Locales; }

bool Localization::AddToLocale(std::string LocaleCode, std::string Key, std::string Value)
{
	for (LocalizationData& Locale : Framework::Locales)
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

bool Localization::AddToLocale(std::string LocaleCode, std::vector<LocaleData> arrLocaleData)
{
	for (LocalizationData& Locale : Framework::Locales)
	{
		if (Locale.LocaleCode == HASH(LocaleCode))
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
	Framework::CurrentLocale.LocaleCode;

	for (LocalizationData Locale : Framework::Locales)
	{
		if (Locale.LocaleCode == Framework::CurrentLocale.LocaleCode)
		{
			Framework::CurrentLocale = Locale;
			return true;
		}
	}

	return false;
}