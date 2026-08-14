#pragma once
#include "English.hpp"
#include "German.hpp"
#include "Polish.hpp"

// To add a locale: create Locales/<Name>.hpp modeled on English.hpp, #include it above,
// and register it here. No other source file needs to change.
//
// Called from Localization's own constructor, while GetInstance()'s lazy singleton init is
// still in progress - so this loads locales directly onto the instance being built (self)
// rather than through Localization::LoadLocale()/GetInstance(). Re-entering GetInstance() here
// would deadlock: MSVC's thread-safe magic-static guard blocks a thread that recursively waits
// on its own in-progress initialization.
inline void RegisterLocales(Localization& self)
{
	self._LoadLocale(localeEnglish);
	self._LoadLocale(localeGerman);
	self._LoadLocale(localePolish);
}
