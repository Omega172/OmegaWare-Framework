#pragma once
#include <string>
#include <vector>
#include <functional>

#define HASH(str) std::hash<std::string>{}(str)

struct LocaleData
{
	size_t Key;
	std::string Value;
};

struct LocalizationData
{
	std::string Name;
	size_t LocaleCode;

	std::vector<LocaleData> Locales;
};