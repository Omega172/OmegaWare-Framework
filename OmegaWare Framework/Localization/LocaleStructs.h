#pragma once
#include <string>
#include <vector>

struct LocaleData
{
	std::string Key;
	std::string Value;
};

struct LocalizationData
{
	std::string Name;
	std::string LocalCode;

	std::vector<LocaleData> Locals;
};