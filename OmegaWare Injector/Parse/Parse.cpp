#pragma once
#include "Parse.h"
#include "../Utils/Utils.h"

Parse::Parse(int iCount, char** pInput)
{
	Count = iCount;
	Input = pInput;
}

Parse::ParseData Parse::GetValue(std::string sKey)
{
	ParseData pd;
	for (int i = 0; i < Count; i++)
	{
		std::string s = Input[i];
		s = Utils::ToLower(s);

		if (s.find(sKey) != std::string::npos)
		{
			pd.bFound = true;
			pd.sKey = sKey;
			pd.sValue = s.substr(s.find(" ") + 1);
			break;
		}
	}
	return pd;
}

Parse::ParseData Parse::FindValue(std::string sKey)
{
	ParseData pd;
	for (int i = 0; i < Count; i++)
	{
		std::string s = Input[i];
		s = Utils::ToLower(s);

		if (s.find(sKey) != std::string::npos)
		{
			pd.bFound = true;
			pd.sKey = sKey;
			break;
		}
	}
	return pd;
}