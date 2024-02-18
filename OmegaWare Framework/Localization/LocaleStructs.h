#pragma once
#include <string>
#include <vector>
#include <functional>

struct LocaleData
{
	size_t Key;
	std::string Value;
};

struct LocalizationData
{
	std::string Name;
	size_t LocalCode;

	std::vector<LocaleData> Locals;
};