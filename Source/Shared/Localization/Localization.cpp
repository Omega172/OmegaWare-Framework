#include "pch.h"

#include "Locales/Locales.hpp"

Localization::Localization()
{
	RegisterLocales(*this);

	if (!_SetLocale("ENG"Hashed))
		Utils::LogError("Localization::_SetLocale failure!");
};

std::string Localization::_Get(const size_t ullKeyHash) const
{
	if (m_iCurrentLocale < m_Locales.size())
	{
		Locale_t stCurrentLocale = m_Locales.at(m_iCurrentLocale);

		auto itrLocalizedStrings = stCurrentLocale.umLocalizedStrings.find(ullKeyHash);
		if (itrLocalizedStrings != stCurrentLocale.umLocalizedStrings.end())
			return itrLocalizedStrings->second;
	}

	constexpr size_t ullENGKeyHash = "ENG"Hashed;
	for (Locale_t stLocale : m_Locales)
	{
		if (stLocale.ullKeyHash != ullENGKeyHash)
			continue;

		auto itrLocalizedStrings = stLocale.umLocalizedStrings.find(ullKeyHash);
		if (itrLocalizedStrings != stLocale.umLocalizedStrings.end())
			return itrLocalizedStrings->second;

	}

	// @TODO make this return the string we are trying to lookup
	return "NOT_FOUND";
}

void Localization::_LoadLocale(Locale_t& _stLocale)
{
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

	m_Locales.push_back(_stLocale);
}

bool Localization::_SetLocale(size_t ullKeyHash)
{
	for (size_t i = 0; i < m_Locales.size(); ++i)
	{
		Locale_t& stLocale = m_Locales.at(i);

		if (stLocale.ullKeyHash != ullKeyHash)
			continue;

		m_iCurrentLocale = i;

		CurrentFont = *stLocale.hMenuFont;

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

	m_Locales.emplace_back(Locale_t({
		.sKey = sLocaleKey,
		.ullKeyHash = ullLocaleKeyHash,

		.hMenuFont = &TahomaFont,

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
