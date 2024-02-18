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

	bool SetLocale(std::string LocalCode);

	std::vector<LocalizationData> GetLocales();

	bool AddToLocale(std::string LocalCode, std::string Key, std::string Value);
};