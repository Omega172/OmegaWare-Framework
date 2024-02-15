#pragma once
#include <string>

class Parse
{
private:
	int Count;
	char** Input;

public:
	Parse(int Count, char** Input);

	struct ParseData
	{
		bool bFound = false;
		std::string sKey = "";
		std::string sValue = "";
	};

	std::string GetArg(int iIndex) { return Input[iIndex]; }

	ParseData GetValue(std::string sKey);
	ParseData FindValue(std::string sKey);
};