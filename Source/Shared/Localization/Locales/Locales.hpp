#pragma once
#include "English.hpp"
#include "German.hpp"
#include "Polish.hpp"

// To add a locale: create Locales/<Name>.hpp modeled on English.hpp, #include it above,
// and register it here. No other source file needs to change.
inline void RegisterLocales(Localization& self)
{
	self._LoadLocale(localeEnglish);
	self._LoadLocale(localeGerman);
	self._LoadLocale(localePolish);
}
