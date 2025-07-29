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
#include <memory>

#ifdef _WIN32
#include <windows.h>
#define getCurThreadtid() GetCurrentThreadId()
#else
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/time.h>
#define getCurThreadtid() pthread_self()
#endif
enum LogLevel{
    EM_LOG_TRACE = 0,
    EM_LOG_DEBUG,
    EM_LOG_INFO,
    EM_LOG_WARNING,
    EM_LOG_ERROR,
    EM_LOG_FATAL
};
#define LOG_TRACE(format, ...)   FileLogger::getInstance().log(EM_LOG_TRACE,__func__, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)   FileLogger::getInstance().log(EM_LOG_DEBUG,__func__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)    FileLogger::getInstance().log(EM_LOG_INFO,__func__, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...) FileLogger::getInstance().log(EM_LOG_WARNING,__func__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)   FileLogger::getInstance().log(EM_LOG_ERROR,__func__, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...)   FileLogger::getInstance().log(EM_LOG_FATAL,__func__, format, ##__VA_ARGS__)

using namespace std;
class FileLogger {
public:
    static FileLogger& getInstance();
    void setLogLevel(LogLevel level);
    void initLog(const std::string &strCfgName);
    void setLogFileName(const std::string& strFileName);
    template<typename... Args>
    void log(LogLevel emLevel, const char* szFun, const char* format, Args... args)
    {
        if (emLevel < m_emLogLevel)
            return;
      
        try {           
            std::string strFormatted = formatMessage(emLevel, szFun, format, args...);
                       
            if (strFormatted[strFormatted.length() - 1] != '\n')
                strFormatted += '\n';

            if (m_bOutPutFile)
                writeToFile(strFormatted);
            else
                writeToConsole(emLevel,strFormatted);
    
        }
        catch (const std::exception& e) {
            std::cerr << "Log formatting error: " << e.what() << std::endl;
        }
    }
private:
    FileLogger(const char *strBase = "log.log",
        size_t maxSize = 20 * 1024 * 1024,
        int maxFiles = 30,
        LogLevel level = EM_LOG_DEBUG);

    ~FileLogger();
    void parseFileNameComponents();
    template<typename... Args>
    std::string formatMessage(LogLevel emLevel, const char* szFun, const char* format, Args... args) {
        std::string strContent = stringFormat(format, args...);

        char szTimeBuf[32];
#if defined(_WIN32)
        SYSTEMTIME t;
        GetLocalTime(&t);
        snprintf(szTimeBuf, sizeof(szTimeBuf), "%04hu-%02hu-%02hu %02hu:%02hu:%02hu.%03hu", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
#else
        struct tm t;
        struct timeval tv;
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &t);
        snprintf(szTimeBuf, sizeof(szTimeBuf), "%04d-%02d-%02d %02d:%02d:%02d.%03d", (int)t.tm_year + 1900, (int)t.tm_mon + 1, (int)t.tm_mday, (int)t.tm_hour, (int)t.tm_min, (int)t.tm_sec, (int)(tv.tv_usec / 1000) % 1000);
#endif

        const char* strLevel = nullptr;
        switch (emLevel) {
        case EM_LOG_TRACE:   strLevel = "TRACE"; break;
        case EM_LOG_DEBUG:   strLevel = "DEBUG"; break;
        case EM_LOG_INFO:    strLevel = "INFO ";  break;
        case EM_LOG_WARNING: strLevel = "WARN ";  break;
        case EM_LOG_ERROR:   strLevel = "ERROR"; break;
        case EM_LOG_FATAL:   strLevel = "FATAL"; break;
        }
        uint32_t uThreadId = getCurThreadtid();
        return stringFormat("[%s] [%s] [tid:%05d] [%s] %s", szTimeBuf, strLevel, uThreadId, szFun,  strContent.c_str());
    }

    std::string stringFormat(const char* format, ...);

    void writeToFile(const std::string& message);

    void writeToConsole(LogLevel emLevel,const std::string& message);

    void openCurrentFile();

    void rotateFiles();

    void purgeOldFiles();

    int findMaxFileIndex();

    void processFileName(const std::string& fileName, std::vector<int>& indices);
    bool isNumber(const std::string& s);

    std::string generateFileName(int index) const;
private:
    static FileLogger m_sFileLogger;
    FILE* n_hFile;
    std::string m_strBaseName;
    std::string m_strFilePrefix;
    std::string m_strFileExt;
    std::string m_strCurrentFile;
    size_t m_iMaxSize;
    int m_iMaxFiles;
    size_t m_iCurrentSize;
    LogLevel m_emLogLevel;
    size_t m_iCurrentIndex;
    bool m_bOutPutFile;
    std::mutex m_Mutex;
};
#endif