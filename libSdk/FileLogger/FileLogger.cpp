/*
Written by Nack li <nackli@163.com>
Copyright (c) 2024. All Rights Reserved.
*/

#include "FileLogger.h"
#include <stdio.h>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>
#include "../Common/FileSystem.h"
#include "../Common/StringUtils.h"
#include "../Common/LockQueue.h"
#include "../mem/ConcurrentMem.h"
#include "formatPattern.h"
#include "OutPutMode.h"


#ifdef _WIN32
#define UNUSED_FUN
#else
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<arpa/inet.h>
#include <thread>
#include <unistd.h>
#define UNUSED_FUN __attribute__((unused))
#endif

#define FILE_CLOSE(x)					if((x)){if(!FileSystem::closeFile((x)))(x) = nullptr;}
#define OUT_CONSOLE                     0x0
#define OUT_LOC_FILE                    0x01
#define OUT_NET_UDP                     0x02

#ifndef INVALID_SOCKET
#define INVALID_SOCKET                  0x0
#endif

#define FREE_MEM(x)                 if((x)) {PM_FREE((x)); (x)=nullptr;}
FileLogger FileLogger::m_sFileLogger;
UNUSED_FUN static void memory_dump(const void* ptr, unsigned int len)
{
    unsigned int i, j;
    const unsigned char* p;

    p = (const unsigned char*)ptr;
    for (j = 0; j < len; j += 16)
    {
        for (i = 0; i < 16; i++)
            printf("%02x ", i + j < len ? (unsigned int)p[i + j] : 0);

        for (i = 0; i < 16; i++)
            printf("%c", i + j < len ? p[i + j] : ' ');

        printf("\n");
    }
}

static std::map<std::string, std::string> parseConfig(const std::string& path) 
{
    std::map<std::string, std::string> config;
    if (path.empty())
        return config;

    std::ifstream file(path);
    std::string line;

    while (std::getline(file, line))
    {
        std::string strData = subLeft(line, "#");
        if (strData.empty())
            continue;
        std::pair <std::string, std::string> pairKv = spiltKv(strData);
        if(!pairKv.first.empty())
            config[pairKv.first] = pairKv.second;
    }
    return config;
}


UNUSED_FUN static int dir_list(const char* szDir, int (CallFunFileList)(void* param, const char* name, int isdir), void* param)
{
    int r;
#if _WIN32
    BOOL next;
    HANDLE handle;
    WIN32_FIND_DATAA data;
    char pathext[MAX_PATH];

    // dir with wildchar
    r = snprintf(pathext, sizeof(pathext), "%s\\*", szDir);
    if (r >= sizeof(pathext) || r < 1)
        return -1;

    handle = FindFirstFileA(pathext, &data);
    if (handle == INVALID_HANDLE_VALUE)
        return -1;

    next = TRUE;
    for (r = 0; 0 == r && next; next = FindNextFileA(handle, &data))
    {
        if (0 == strcmp(data.cFileName, ".") || 0 == strcmp(data.cFileName, ".."))
            continue;

        r = CallFunFileList(param, data.cFileName, (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0);
    }

    FindClose(handle);
    return r;
#else
    DIR* dir;
    struct dirent* p;

    dir = opendir(szDir);
    if (!dir)
        return -(int)errno;

    r = 0;
    for (p = readdir(dir); p && 0 == r; p = readdir(dir))
    {
        if (0 == strcmp(p->d_name, ".") || 0 == strcmp(p->d_name, ".."))
            continue;

        r = CallFunFileList(param, p->d_name, DT_DIR == p->d_type ? 1 : 0);
    }

    closedir(dir);
    return r;
#endif
}

FileLogger& FileLogger::getInstance() {
    return m_sFileLogger;
}

void FileLogger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_emLogLevel = level;
}


void FileLogger::initLog(const std::string &strCfgName)
{
    string strBaseName("./Logs/log.log");
    int iMaxFileNum = 10;
    int iMaxFileSize = 0x500000;
    string strNetIpAddr ="127.0.0.1";
    int iNetPort = 0;
    int iOutPutFile = OUT_LOC_FILE;
    string strLogFormat("[%D] [%l] [tid:%t] [%!] %v%n");

    if (!strCfgName.empty())
    {
        std::map<std::string, std::string> mapCfg = parseConfig(strCfgName);
        if (!mapCfg.empty())
        {
            if (!mapCfg["file_name"].empty())
                strBaseName = mapCfg["file_name"];
            if(!mapCfg["max_files"].empty())
                iMaxFileNum = str2Int(mapCfg["max_files"]);
            if(!mapCfg["max_size"].empty())
                iMaxFileSize = str2Uint(mapCfg["max_size"]);
            if (!mapCfg["log_level"].empty())
                m_emLogLevel = OnStringToLevel(mapCfg["log_level"]);
            if (!mapCfg["out_put"].empty())
            {
                if (equals(mapCfg["out_put"].c_str(), "console", false))
                    iOutPutFile = OUT_CONSOLE;
                else if(equals(mapCfg["out_put"].c_str(), "netudp", false))
                    iOutPutFile = OUT_NET_UDP;
                else
                    iOutPutFile = OUT_LOC_FILE;
            }
            if (!mapCfg["out_mode"].empty())
                m_bSync = equals(mapCfg["out_mode"].c_str(), "sync", false);
            if (!mapCfg["log_pattern"].empty())
                strLogFormat = mapCfg["log_pattern"];

            if (iOutPutFile == OUT_NET_UDP)
            {
                if (!mapCfg["netIp"].empty())
                {
                    std::pair<std::string, std::string> pairNet = spiltKv(mapCfg["netIp"], ':');
                    if (!pairNet.first.empty() && !pairNet.second.empty())
                    {
                        strNetIpAddr = pairNet.first;
                        iNetPort = str2Int(pairNet.second);
                    }
                    else
                        iOutPutFile = OUT_LOC_FILE;
                }
                else
                    iOutPutFile = OUT_LOC_FILE;
            }
        }
    }

    m_pPatternFmt = new FormatterBuilder(strLogFormat);

    if (iOutPutFile == OUT_NET_UDP)
    {
        m_pOutputMode = new UdpOutPutMode;
        m_pOutputMode->initOutMode(strNetIpAddr.c_str(), iNetPort);
    }

    else if (iOutPutFile == OUT_LOC_FILE)
    {
        m_pOutputMode = new FileOutPutMode;
        m_pOutputMode->initOutMode(strBaseName.c_str(), iMaxFileSize);
        m_pOutputMode->setMaxFileNum(iMaxFileNum);
    } 
    else
        m_pOutputMode = new ConsoleOutPutMode;

    if (!m_bSync)
    {
        std::thread tRead(&FileLogger::outPut2File, this);
        tRead.detach();
    }

}

void FileLogger::setLogFileName(const std::string& strFileName)
{
    if (m_pOutputMode && !strFileName.empty())
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_pOutputMode->changeOutModeCfg(strFileName);
    }
}

FileLogger::FileLogger(LogLevel level)
    :m_emLogLevel(level),
    m_bSync(false),
    m_pOutputMode(nullptr),
    m_pPatternFmt(nullptr)
    
{
    m_ctxQueue.setSpaces(200);
}

FileLogger::~FileLogger() 
{
    if (m_pOutputMode)
    {
        m_pOutputMode->closeOutPut();
        delete m_pOutputMode;
        m_pOutputMode = nullptr;
    }
}

void FileLogger::formatMessage(LogLevel emLevel, const char* szFunName, const char* szFileName, 
    const int iLine, const string & strMessage)
{
    LogMessage logMsg(emLevel, szFileName, iLine, strMessage.c_str(), szFunName);
    memory_buf_t bufDest;
    string strFormatted = m_pPatternFmt->format(logMsg);
    writeToOutPut(emLevel, strFormatted);
}

void FileLogger::outPut2File()
{
    while (1)
    {     
        do 
        {
            std::string strData = m_ctxQueue.pop_front();
            m_pOutputMode->writeData(strData,0); 
        }while(!m_ctxQueue.empty());
        //m_pOutputMode->flushFile();
    }
}

void FileLogger::writeToOutPut(LogLevel &emLevel, const std::string& strMsg)
{
    if (!m_bSync)
        m_ctxQueue.push(strMsg);
    else
    {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_pOutputMode->writeData(strMsg, emLevel);
        }

        //m_pOutputMode->flushFile();
    }
}
