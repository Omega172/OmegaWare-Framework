#pragma once

#include "LocaleStructs.h"
#include "Libs/CRC64/CRC64.h"

class Localization
{
private:
	bool bInitialized = false;

public:
	Localization();

	bool IsInitialized();

	std::string Get(std::string Key);
	std::string Get(size_t ullKeyHash);

	static std::string StaticGet(size_t ullKeyHash);

	void LoadLocale(LocalizationData Locale);

	bool SetLocale(std::string LocaleCode);
	bool SetLocale(size_t LocaleCodeHash);

	std::vector<LocalizationData> GetLocales();

	bool AddToLocale(std::string LocaleCode, std::string Key, std::string Value);
	bool AddToLocale(std::string LocaleCode, std::vector<LocaleData> arrLocaleData);

	bool UpdateLocale();
};