#pragma once

#include <string>
#include <vector>

#include "../Libs/CRC64/CRC64.h"
#include "pch.h"
#include <stdexcept>

// http://zuga.net/articles/unicode-all-characters-supported-by-the-font-tahoma/
const ImWchar DefaultRanges[] = { 0x0020, 0x00FF, 0x0 };
const ImWchar PolishRanges[] = { 0x0020, 0x00FF, 0x0100, 0x017F, 0x0 };

typedef struct Locale_t
{
	std::string sKey;
	size_t ullKeyHash;

	ImFont** hMenuFont;
	ImFont** hFeatureFont;

	std::unordered_map<size_t, std::string> umLocalizedStrings;
} Locale_t;

class Localization
{
public:
	static Localization* GetInstance()
	{
		static Localization* pSingleton = new Localization();
		return pSingleton;
	}

private:
	std::vector<Locale_t> m_Locales;
	size_t m_iCurrentLocale = SIZE_MAX;

	Localization();

	// Internal representation of static functions
	std::string _Get(const size_t ullKeyHash) const;

	void _LoadLocale(Locale_t& stLocale);
	bool _SetLocale(size_t ullKeyHash);

	const std::vector<Locale_t> _GetLocales() const;
	const size_t _GetCurrentLocaleIndex() const;

	void _AddToLocale(std::string sLocaleKey, size_t ullKeyHash, std::string sLocalizedString);
	void _AddToLocale(std::string sLocaleKey, std::initializer_list<std::pair<size_t, std::string>> ilLocalizedStrings);


	// Public functions just get the single localization instance and calls the internal version of the function
public:
	static std::string Get(const std::string sKey);
	static std::string Get(const size_t ullKeyHash);

	static void LoadLocale(Locale_t& stLocale);

	static bool SetLocale(const std::string sKey);
	static bool SetLocale(const size_t ullKeyHash);

	static const std::vector<Locale_t> GetLocales();
	static const size_t GetCurrentLocaleIndex();

	static void AddToLocale(std::string sLocaleKey, size_t ullKeyHash, std::string sLocalizedString);
	static void AddToLocale(std::string sLocaleKey, std::initializer_list<std::pair<size_t, std::string>> ilLocalizedStrings);
};