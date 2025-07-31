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
#include "LockQueue.h"

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
#define LOG_BASE(Level,format, ...)     FileLogger::getInstance().log(Level,__func__,__FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOG_TRACE(format, ...)          LOG_BASE(EM_LOG_TRACE, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...)          LOG_BASE(EM_LOG_DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)           LOG_BASE(EM_LOG_INFO, format, ##__VA_ARGS__)
#define LOG_WARNING(format, ...)        LOG_BASE(EM_LOG_WARNING, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...)          LOG_BASE(EM_LOG_ERROR, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...)          LOG_BASE(EM_LOG_FATAL, format, ##__VA_ARGS__)

using namespace std;
class FileLogger {
public:
    static FileLogger& getInstance();
    void setLogLevel(LogLevel level);
    void initLog(const std::string &strCfgName);
    void setLogFileName(const std::string& strFileName);

    template<typename... Args>
    void log(LogLevel emLevel, const char* szFun,const char *szFileName, const int iLine, const char* format, Args&&... args)
    {
        if (emLevel < m_emLogLevel)
            return;

        try {           
            std::string strFormatted = formatMessage(emLevel, szFun, szFileName, iLine, format, std::forward<Args>(args)...);
                       
            if (strFormatted[strFormatted.length() - 1] != '\n')
                strFormatted += '\n';

            writeToOutPut(emLevel,strFormatted);
    
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
    std::string formatMessage(LogLevel emLevel, const char* szFun, const char* szFileName, const int iLine, const char* format, Args&&... args) {
        std::string strContent = stringFormat(format, std::forward<Args>(args)...);

        return formatMessage(emLevel, szFun, szFileName, iLine,strContent.c_str());
    }

    std::string stringFormat(const char* format, ...);

    std::string formatMessage(LogLevel emLevel, const char* szFunName, const char* szFileName, const int iLine, const char* szMessage);

    void writeToOutPut(LogLevel emLevel,const std::string& message);
    void outPut2File(int iOutMode);
    void writeMsg2File(const std::string& strMsg);
    void writeMsg2Net(const std::string& strMsg);
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
    int m_iOutPutFile;
    bool m_bSync;
    std::mutex m_Mutex;
    LockQueue<std::string> m_ctxQueue;
    string m_strLogFormat;
//net log
    string m_strNetIpAdd;
    int m_iNetPort;
#ifdef _WIN32
    SOCKET m_hSendSocket;
#else
    int m_hSendSocket;
#endif
};
#endif