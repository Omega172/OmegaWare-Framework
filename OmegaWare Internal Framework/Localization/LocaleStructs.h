#pragma once
#include <string>
#include <vector>
#include "pch.h"

#define HASH(str) std::hash<std::string>{}(str)

static ImWchar DefaultRanges[] = { 0x0020, 0x00FF, 0 };
static ImWchar PolishRanges[] = { 0x0020, 0x00FF, 0x00A0, 0x02D9, 0 };

struct LocaleData
{
	size_t Key;
	std::string Value;
};

struct LocalizationData
{
	std::string Name;
	size_t LocaleCode;
	ImFont** Font;
	ImFont** FontESP;

	std::vector<LocaleData> Locales;
};