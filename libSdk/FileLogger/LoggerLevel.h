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
#ifndef __LOGGER_LEVEL_H_
#define __LOGGER_LEVEL_H_
#include <map>

enum LogLevel{
    EM_LOG_TRACE = 0,
    EM_LOG_DEBUG,
    EM_LOG_INFO,
    EM_LOG_WARN,
    EM_LOG_ERROR,
    EM_LOG_FATAL
};

inline const char* getLoggerLevelName(int iLev)
{
	const char* szLevel = nullptr;
	switch (iLev)
	{
	case EM_LOG_TRACE:   
		szLevel = "TRACE"; 
		break;
	case EM_LOG_DEBUG:   
		szLevel = "DEBUG"; 
		break;
	case EM_LOG_INFO:
		szLevel = "INFO "; 
		break;
	case EM_LOG_WARN:
		szLevel = "WARN ";
		break;
	case EM_LOG_ERROR:
		szLevel = "ERROR";
		break;
	case EM_LOG_FATAL:
		szLevel = "FATAL";
		break;
	default:
		szLevel = "DEBUG";
		break;
	}
	return szLevel;
}

inline const char * getLoggerLevelShortName(int iLev)
{
	const char* szLevel = nullptr;
	switch (iLev)
	{
	case EM_LOG_TRACE:   
		szLevel = "T"; 
		break;
	case EM_LOG_DEBUG:   
		szLevel = "D"; 
		break;
	case EM_LOG_INFO:
		szLevel = "I"; 
		break;
	case EM_LOG_WARN:
		szLevel = "W";
		break;
	case EM_LOG_ERROR:
		szLevel = "E";
		break;
	case EM_LOG_FATAL:
		szLevel = "F";
		break;
	default:
		szLevel = "D";
		break;
	}
	return szLevel;
}

inline LogLevel OnStringToLevel(const std::string& strLevel) {
    static std::map<std::string, LogLevel> levelMap = {
        {"TRACE", LogLevel::EM_LOG_TRACE},
        {"DEBUG", LogLevel::EM_LOG_DEBUG},
        {"INFO", LogLevel::EM_LOG_INFO},
        {"WARN", LogLevel::EM_LOG_WARN},
        {"ERROR", LogLevel::EM_LOG_ERROR},
        {"FATAL", LogLevel::EM_LOG_FATAL}
    };
	std::string strLev = strLevel;
    toUpper(strLev);
	if(levelMap.find(strLev) == levelMap.end())
		return LogLevel::EM_LOG_DEBUG;
    return levelMap.at(strLev);
}
#endif