#pragma once
#include "LocaleStructs.h"

class Localization
{
private:
	bool bInitialized = false;

public:
	Localization();

	bool IsInitialized();

	std::string Get(std::string Key);

	void LoadLocale(LocalizationData Locale);

	bool SetLocale(std::string LocaleCode);
	bool SetLocale(size_t LocaleCodeHash);

	std::vector<LocalizationData> GetLocales();

	bool AddToLocale(std::string LocaleCode, std::string Key, std::string Value);

	bool UpdateLocale();
};