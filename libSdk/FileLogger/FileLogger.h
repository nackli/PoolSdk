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
#ifndef __PLATFORM_FILE_LOG_H_
#define __PLATFORM_FILE_LOG_H_
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <ctime>
#include <mutex>
//#include <pair>
#include <memory>
#include "../Common/LockQueue.h"
#include "../Common/StringUtils.h"
#include "fmt/printf.h"
#include "LoggerLevel.h"
#ifdef _WIN32
#include <windows.h>
#define getCurThreadtid() GetCurrentThreadId()
#else
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/time.h>
#define getCurThreadtid() gettid()
#endif
//enum LogLevel{
//    EM_LOG_TRACE = 0,
//    EM_LOG_DEBUG,
//    EM_LOG_INFO,
//    EM_LOG_WARN,
//    EM_LOG_ERROR,
//    EM_LOG_FATAL
//};
#define LOG_BASE(Level,format, ...)         FileLogger::getInstance().log(true,Level,__func__, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_TRACE(format, ...)              LOG_BASE(EM_LOG_TRACE, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)              LOG_BASE(EM_LOG_DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)               LOG_BASE(EM_LOG_INFO, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)               LOG_BASE(EM_LOG_WARN, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)              LOG_BASE(EM_LOG_ERROR, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...)              LOG_BASE(EM_LOG_FATAL, format, ##__VA_ARGS__)
#define LOG_BASE_S(Level,format, ...)       FileLogger::getInstance().log(false,Level,__func__, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_TRACE_S(format, ...)            LOG_BASE_S(EM_LOG_TRACE, format, ##__VA_ARGS__)
#define LOG_DEBUG_S(format, ...)            LOG_BASE_S(EM_LOG_DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO_S(format, ...)             LOG_BASE_S(EM_LOG_INFO, format, ##__VA_ARGS__)
#define LOG_WARN_S(format, ...)             LOG_BASE_S(EM_LOG_WARN, format, ##__VA_ARGS__)
#define LOG_ERROR_S(format, ...)            LOG_BASE_S(EM_LOG_ERROR, format, ##__VA_ARGS__)
#define LOG_FATAL_S(format, ...)            LOG_BASE_S(EM_LOG_FATAL, format, ##__VA_ARGS__)

class FormatterBuilder;
class OutPutMode;
//using memory_buf_t = fmt::basic_memory_buffer<char, 250>;
using namespace std;
class FileLogger {
public:
    static FileLogger& getInstance();
    void setLogLevel(LogLevel level);
    void initLog(const std::string& strCfgName);
    void setLogFileName(const std::string& strFileName);
    void closeLog();

    template<typename FormatStr,typename... Args>
    void log(bool bFormat,LogLevel emLevel, const char* szFun,const char *szFileName, 
        const int iLine, const FormatStr& format, Args&&... args)
    {
        if (emLevel < m_emLogLevel)
            return;         
        formatMessage(bFormat,emLevel, szFun, szFileName, iLine, format,
            std::forward<Args>(args)...);
    }

private:
    FileLogger(LogLevel level = EM_LOG_DEBUG);

    ~FileLogger();

    template<typename FormatStr,typename... Args>
    void formatMessage(bool bFormat,LogLevel emLevel, const char* szFun, const char* szFileName,
        const int iLine, const FormatStr& formatValue, Args&&... args)
    {
        std::string strContent;
        if(bFormat)
            strContent = fmt::sprintf(formatValue, /*std::forward<Args>*/(args)...); 
        else
        {
            fmt::memory_buffer outBuf;
            fmt::format_to(fmt::appender(outBuf), fmt::runtime(formatValue), std::forward<Args>(args)...);
            strContent = std::string(outBuf.data(), outBuf.size());
        }
        formatMessage(emLevel, szFun, szFileName, iLine, strContent);
    }

    void formatMessage(LogLevel emLevel, const char* szFunName, 
        const char* szFileName, const int iLine, const string& szMessage);

    void writeToOutPut(LogLevel &emLevel,const std::string& message);
    void outPut2File();
private:
    static FileLogger m_sFileLogger;
    LogLevel m_emLogLevel;
    bool m_bSync;
    std::mutex m_Mutex;
    LockQueue<std::pair<std::string,int>> m_ctxQueue;
    OutPutMode* m_pOutputMode;
    FormatterBuilder* m_pPatternFmt;
};
#endif