#include "pch.h"
#include "Localization.h"
#include "Locales/English.h"
#include "Locales/German.h"
#include "Locales/Polish.h"

// Private functions

Localization::Localization()
{
	_LoadLocale(localeEnglish);
	_LoadLocale(localeGerman);
	_LoadLocale(localePolish);

	if (!_SetLocale("ENG"Hashed))
		LogErrorHere("Failed to set default locale!");
};

std::string Localization::_Get(const size_t ullKeyHash) const
{
	// Check that the current locale index is within the bounds of the locale vector
	if (m_iCurrentLocale < m_Locales.size())
	{
		Locale_t stCurrentLocale = m_Locales.at(m_iCurrentLocale);

		// Look into the currently active locale's unordered_map of localized strings for this entry
		auto itrLocalizedStrings = stCurrentLocale.umLocalizedStrings.find(ullKeyHash);
		if (itrLocalizedStrings != stCurrentLocale.umLocalizedStrings.end())
			return itrLocalizedStrings->second;
	}

	// If we could not find the localized string in the current locale, search for it in the english locale
	constexpr size_t ullENGKeyHash = "ENG"Hashed;
	for (Locale_t stLocale : m_Locales)
	{
		if (stLocale.ullKeyHash != ullENGKeyHash)
			continue;

		auto itrLocalizedStrings = stLocale.umLocalizedStrings.find(ullKeyHash);
		if (itrLocalizedStrings != stLocale.umLocalizedStrings.end())
			return itrLocalizedStrings->second;

	}

	// If we couldnt find the localized string, we return "NOT_FOUND"
	// @TODO make this return the string we are trying to lookup
	return "NOT_FOUND";
}

void Localization::_LoadLocale(Locale_t& _stLocale)
{
	// If this locale already exists, we just transfer the data to the already existing entry
	for (Locale_t& stLocale : m_Locales)
	{
		if (stLocale.ullKeyHash != _stLocale.ullKeyHash)
			continue;

		stLocale.hMenuFont = _stLocale.hMenuFont;
		stLocale.hFeatureFont = _stLocale.hFeatureFont;

		for (auto pair : _stLocale.umLocalizedStrings)
			stLocale.umLocalizedStrings.try_emplace(pair.first, pair.second);

		return;
	}

	// Otherwise we just append this new locale to the vector of locales
	m_Locales.push_back(_stLocale);
}

bool Localization::_SetLocale(size_t ullKeyHash)
{

	// Iterate through our locales using a for loop so we can use the index of the locale in the vector for our current locale value
	for (size_t i = 0; i < m_Locales.size(); ++i)
	{
		Locale_t& stLocale = m_Locales.at(i);

		if (stLocale.ullKeyHash != ullKeyHash)
			continue;

		m_iCurrentLocale = i;

		CurrentFont = *stLocale.hMenuFont;
		CurrentFontESP = *stLocale.hFeatureFont;

		return true;
	}

	return false;
}

const std::vector<Locale_t> Localization::_GetLocales() const
{
	return m_Locales;
}

const size_t Localization::_GetCurrentLocaleIndex() const
{
	return m_iCurrentLocale;
}

void Localization::_AddToLocale(std::string sLocaleKey, size_t ullKeyHash, std::string sLocalizedString)
{
	size_t ullLocaleKeyHash = CRC64::hash(sLocaleKey);
	for (Locale_t& stLocale : m_Locales)
	{
		if (stLocale.ullKeyHash != ullLocaleKeyHash)
			continue;

		stLocale.umLocalizedStrings.try_emplace(ullKeyHash, sLocalizedString);
		return;
	}

	// If we find that the current locale was not **yet** created, we just create a dummy locale and add this localized string
	m_Locales.emplace_back(Locale_t({
		.sKey = sLocaleKey,
		.ullKeyHash = ullKeyHash,

		.hMenuFont = &TahomaFont,
		.hFeatureFont = &TahomaFontESP,

		.umLocalizedStrings = std::unordered_map<size_t, std::string>({
			{ ullKeyHash, sLocalizedString },
		}),
		}));
}

void Localization::_AddToLocale(std::string sLocaleKey, std::initializer_list<std::pair<size_t, std::string>> ilLocalizedStrings)
{
	// Lazy solution that does alot of extra redundant work...
	for (auto itr = ilLocalizedStrings.begin(); itr != ilLocalizedStrings.end(); ++itr)
		_AddToLocale(sLocaleKey, itr->first, itr->second);
}


// public functions

std::string Localization::Get(const std::string sKey)
{
	return Get(CRC64::hash(sKey));
}

std::string Localization::Get(const size_t ullKeyHash)
{
	return GetInstance()->_Get(ullKeyHash);
}

void Localization::LoadLocale(Locale_t& stLocale)
{
	GetInstance()->_LoadLocale(stLocale);
}

bool Localization::SetLocale(const std::string sKey)
{
	return SetLocale(CRC64::hash(sKey));
}

bool Localization::SetLocale(const size_t ullKeyHash)
{
	return GetInstance()->_SetLocale(ullKeyHash);
}

const std::vector<Locale_t> Localization::GetLocales()
{
	return GetInstance()->_GetLocales();
}

const size_t Localization::GetCurrentLocaleIndex()
{
	return GetInstance()->_GetCurrentLocaleIndex();
}

void Localization::AddToLocale(std::string sLocaleKey, size_t ullKeyHash, std::string sLocalizedString)
{
	GetInstance()->_AddToLocale(sLocaleKey, ullKeyHash, sLocalizedString);
}

void Localization::AddToLocale(std::string sLocaleKey, std::initializer_list<std::pair<size_t, std::string>> ilLocalizedStrings)
{
	GetInstance()->_AddToLocale(sLocaleKey, ilLocalizedStrings);
}
