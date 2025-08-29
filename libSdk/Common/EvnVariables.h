/***************************************************************************************************************************************************/
/*
* @Author: Nack Li
* @version 1.0
* @copyright 2025 nackli. All rights reserved.
* @License: MIT (https://opensource.org/licenses/MIT).
* @Date: 2025-08-29
* @LastEditTime: 2025-08-29
*/
/***************************************************************************************************************************************************/
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

