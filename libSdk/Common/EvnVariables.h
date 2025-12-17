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
class EvnVariables
{
public:
	EvnVariables();
	~EvnVariables();
	bool setValue(const std::string& strKey, const std::string& strValue);
	std::string getValue(const std::string& strKey);
	bool delValue(const std::string& strKey);
};
#endif

