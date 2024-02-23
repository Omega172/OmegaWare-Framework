#pragma once
#include <string>
#include <vector>
#include "pch.h"

#define HASH(str) std::hash<std::string>{}(str)

static const ImWchar DefaultRanges[] = { 0x0020, 0x00FF, 0 };
static const ImWchar PolishRanges[] = { 0x0020, 0x00FF, 0x00A0, 0x02D9, 0 };

struct LocaleData
{
	size_t Key;
	std::string Value;
};

struct LocalizationData
{
	std::string Name;
	size_t LocaleCode;
	const ImWchar* GlyphRanges;

	std::vector<LocaleData> Locales;
};