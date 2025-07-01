#pragma once
#ifndef _PLATFORM_EVN_VAR_H_
#define _PLATFORM_EVN_VAR_H_
#include <string>
using namespace std;
class EvnVariables
{
public:
	EvnVariables();
	~EvnVariables();
	bool setValue(const string& strKey, const string& strValue);
	string getValue(const string& strKey);
	bool delValue(const string& strKey);
};
#endif

