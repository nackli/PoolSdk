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
#include "FileSystem.h"
#include "StringUtils.h"
#include "LockQueue.h"
#ifdef _WIN32
#define UNUSED_FUN
#else
#include<netinet/in.h>
#include <sys/types.h>
#include<sys/socket.h>
#define UNUSED_FUN __attribute__((unused))
#endif
#define FILE_CLOSE(x)					if((x)){if(!fclose((x)))(x) = nullptr;}
#define OUT_CONSOLE                     0x0
#define OUT_LOC_FILE                    0x01
#define OUT_NET_UDP                     0x02

#ifndef INVALID_SOCKET
#define INVALID_SOCKET                  0x0
#endif

#define FREE_MEM(x)                 if((x)) {free((x)); (x)=nullptr;}
FileLogger FileLogger::m_sFileLogger;
#ifdef _WIN32
#pragma comment(lib,"Ws2_32.lib")
#endif

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

static void OnCreateDirFromFilePath(const string strFilePath)
{
    string strCfgDir = getDirFromFilePath(strFilePath);
    if (!IsDirectoryExists(strCfgDir))
        createDirectoryRecursive(strCfgDir);
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

#ifdef _WIN32
static SOCKET OnCreateSocket()
#else
static int OnCreateSocket()
#endif
{
#ifdef _WIN32
    WSADATA wsaData;
    int  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        throw std::runtime_error("WSAStartup failed with error\n");
        return INVALID_SOCKET;
    }
    //---------------------------------------------
    // Create a socket for sending data
    SOCKET hSendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (hSendSocket == INVALID_SOCKET) {
        throw std::runtime_error("socket failed\n");
        WSACleanup();
        return INVALID_SOCKET;
    }
#else
    int hSendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (hSendSocket < 0)
        throw std::runtime_error("socket creation failed");
#endif
    return hSendSocket;
}

static LogLevel OnStringToLevel(const std::string& strLevel) {
    static std::map<std::string, LogLevel> levelMap = {
        {"TRACE", LogLevel::EM_LOG_TRACE},
        {"DEBUG", LogLevel::EM_LOG_DEBUG},
        {"INFO", LogLevel::EM_LOG_INFO},
        {"WARNING", LogLevel::EM_LOG_WARNING},
        {"ERROR", LogLevel::EM_LOG_ERROR},
        {"FATAL", LogLevel::EM_LOG_FATAL}
    };
    string strLev = strLevel;
    toUpper(strLev);
    return levelMap.at(strLev);
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
    if (!strCfgName.empty())
    {
        std::map<std::string, std::string> mapCfg = parseConfig(strCfgName);
        if (!mapCfg.empty())
        {
            if (!mapCfg["file_name"].empty())
                m_strBaseName = mapCfg["file_name"];         
            if(!mapCfg["max_files"].empty())
                m_iMaxFiles = str2Int(mapCfg["max_files"]);
            if(!mapCfg["max_size"].empty())
                m_iMaxSize = str2Uint(mapCfg["max_size"]);
            if (!mapCfg["log_level"].empty())
                m_emLogLevel = OnStringToLevel(mapCfg["log_level"]);
            if (!mapCfg["out_put"].empty())
            {
                if (equals(mapCfg["out_put"].c_str(), "console", false))
                    m_iOutPutFile = OUT_CONSOLE;
                else if(equals(mapCfg["out_put"].c_str(), "netudp", false))
                    m_iOutPutFile = OUT_NET_UDP;
                else
                    m_iOutPutFile = OUT_LOC_FILE;
            }
            if (!mapCfg["out_mode"].empty())
                m_bSync = equals(mapCfg["out_mode"].c_str(), "sync", false);
            if (!mapCfg["log_format"].empty())
                m_strLogFormat = mapCfg["log_format"];

            if (m_iOutPutFile == OUT_NET_UDP)
            {
                if (!mapCfg["netIp"].empty())
                {
                    std::pair<std::string, std::string> pairNet = spiltKv(mapCfg["netIp"], ':');
                    if (!pairNet.first.empty() && !pairNet.second.empty())
                    {
                        m_strNetIpAdd = pairNet.first;
                        m_iNetPort = str2Int(pairNet.second);
                    }
                    else
                        m_iOutPutFile = OUT_LOC_FILE;
                }
                else
                    m_iOutPutFile = OUT_LOC_FILE;
            }
        }
        if (m_iOutPutFile == OUT_LOC_FILE)
        {
            parseFileNameComponents();
            m_iCurrentIndex = findMaxFileIndex();
        }
    }
    if (m_iOutPutFile == OUT_NET_UDP)
        m_hSendSocket = OnCreateSocket();

    if (!m_bSync)
    {
        std::thread tRead(&FileLogger::outPut2File, this, m_iOutPutFile);
        tRead.detach();
    }
}

void FileLogger::setLogFileName(const std::string& strFileName)
{
    if (!strFileName.empty())
    {
        m_strBaseName = strFileName;
        parseFileNameComponents();
        m_iCurrentIndex = findMaxFileIndex();
    }
}

FileLogger::FileLogger(const char* strBase, size_t maxSize, int maxFiles, LogLevel level)
    :n_hFile(nullptr),
    m_strBaseName(strBase),
    m_strFilePrefix(),
    m_strFileExt(),
    m_strCurrentFile(),
    m_iMaxSize(maxSize),
    m_iMaxFiles(maxFiles),
    m_iCurrentSize(0),
    m_emLogLevel(level),
    m_iCurrentIndex(0),
    m_iOutPutFile(OUT_LOC_FILE),
    m_bSync(false),
    m_strLogFormat("[{time}] [{level}] [{tid}] [{func}] {message}"),
    m_hSendSocket(INVALID_SOCKET),
    m_strNetIpAdd(),
    m_iNetPort(0)
{
}

FileLogger::~FileLogger() 
{
    if (m_iOutPutFile == OUT_NET_UDP)
    {
        if (m_hSendSocket != INVALID_SOCKET)
        {
#ifdef _WIN32
            closesocket(m_hSendSocket);
#else
            close(m_hSendSocket);
#endif
            m_hSendSocket = INVALID_SOCKET;
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }
    FILE_CLOSE(n_hFile);
}

void FileLogger::parseFileNameComponents() 
{
    if (m_strBaseName.empty())
        return;
    size_t lastDot = m_strBaseName.rfind('.');
    if (lastDot != std::string::npos) {
        m_strFilePrefix = m_strBaseName.substr(0, lastDot);
        m_strFileExt = m_strBaseName.substr(lastDot);
    }
    else {
        m_strFilePrefix = m_strBaseName;
        m_strFileExt = "";
    }
}

std::string FileLogger::stringFormat(const char* format, ...)
{
    if (!format)
        return {};

    va_list args;
    va_start(args, format);

    int neededSize = vsnprintf(nullptr, 0, format, args) + 1;

    if (neededSize <= 0) {
        va_end(args);
        throw std::runtime_error("Error formatting log message");
    }


    std::unique_ptr<char[]> buf(new char[neededSize]);
    int actualSize = vsnprintf(buf.get(), neededSize, format, args);
    va_end(args);

    if (actualSize < 0)
        throw std::runtime_error("Error formatting log message");


    return std::string(buf.get(), buf.get() + actualSize);
}

static std::string packageMessage(const string & strLogFormat, const char* szTime, const char* szLevel,
                                const char* szTid, const char* szFunName, const char* szFileName,
                                const char* szLine, const string& szMessage)
{

    char *pData = replaceOne(strLogFormat.c_str(), "{level}", szLevel);
    char *pTid = replaceOne(pData, "{tid}", szTid); FREE_MEM(pData);
    char* pLine = replaceOne(pTid, "{line}", szLine); FREE_MEM(pTid);
    char* pFun = replaceOne(pLine, "{func}", szFunName); FREE_MEM(pLine);
    char* pTime= replaceOne(pFun, "{time}", szTime); FREE_MEM(pFun)
    char* pFile = replaceOne(pTime, "{file}", szFileName); FREE_MEM(pTime);
    char *pMsg = replaceOne(pFile, "{message}", szMessage.c_str()); FREE_MEM(pFile);
    std::string result(pMsg);
    FREE_MEM(pMsg);
    return result;
}

std::string FileLogger::formatMessage(LogLevel emLevel, const char* szFunName, const char* szFileName, 
    const int iLine, const string & strMessage)
{
    if (strMessage.empty())
        return {};
    char szTimeBuf[32];
#if defined(_WIN32)
    SYSTEMTIME t;
    GetLocalTime(&t);
    snprintf(szTimeBuf,sizeof(szTimeBuf),"%04hu-%02hu-%02hu %02hu:%02hu:%02hu.%03hu", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);//50ms/6W
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
    char szTid[10] = { 0 };
    if (szTid)
    {
        uint32_t uThreadId = getCurThreadtid(); 
        sprintf(szTid, "%05d", uThreadId);//50MS.6W
    }
    char szLineNum[10] = { 0 };
    sprintf(szLineNum, "%05d", iLine);//50MS.6W
    return packageMessage(m_strLogFormat, szTimeBuf, strLevel, szTid, szFunName,
        szFileName, szLineNum, strMessage);
}

static void OnOutputData(LogLevel &emLevel, const std::string& message)
{
#ifdef _WIN32
    const uint8_t clrIndex[] = {0x0B,0x07,0x08,0x06,0x04,0x05};
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), clrIndex[emLevel]);
    cout << message;
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY);
#else
    const uint8_t *clrIndex[] = { "/033[1;37m","/033[0;37m","/033[0;32;32m", "/033[1;33m","/033[0;32;31m","/033[1;32;31m" };
    printf("%s%s\033[0m", clrIndex[emLevel], message.c_str());
#endif
}

void FileLogger::writeMsg2Net(const std::string& strMsg)
{
    if (strMsg.empty() || m_hSendSocket == INVALID_SOCKET)
        return;
    struct sockaddr_in RecvAddr;
    RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(m_iNetPort);
    RecvAddr.sin_addr.s_addr = inet_addr(m_strNetIpAdd.c_str());
    sendto(m_hSendSocket,
        strMsg.c_str(), strMsg.length() , 0, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));
}

void FileLogger::writeMsg2File(const std::string& strMsg)
{
    if (strMsg.empty())
        return;
    size_t iMsgSize = strMsg.length();

    if (n_hFile == nullptr)
        openCurrentFile();

    if (m_iCurrentSize + iMsgSize > m_iMaxSize)
        rotateFiles();

    if (n_hFile)
    {
        fwrite(strMsg.c_str(), 1, iMsgSize, n_hFile);
        m_iCurrentSize += iMsgSize;
    }
}

void FileLogger::outPut2File(int iOutMode)
{
    while (1)
    {
        while (!m_ctxQueue.empty())
        {
            std::string strData = m_ctxQueue.pop_front();
            if (iOutMode == OUT_NET_UDP)
                writeMsg2Net(strData);
            else
                writeMsg2File(strData);
        }
        if(n_hFile)
            fflush(n_hFile);
    }
}

void FileLogger::writeToOutPut(LogLevel &emLevel, const std::string& strMsg)
{

    if (m_iOutPutFile == OUT_CONSOLE)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        OnOutputData(emLevel, strMsg);
    }
    else
    {
        if (!m_bSync)
            m_ctxQueue.push(strMsg);
        else
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (m_iOutPutFile == OUT_LOC_FILE)
            {
                writeMsg2File(strMsg);
                if (n_hFile)
                    fflush(n_hFile);
            }
            else if (m_iOutPutFile == OUT_NET_UDP)
                writeMsg2Net(strMsg);
        }
    }
}

void FileLogger::openCurrentFile() {
    //std::lock_guard<std::mutex> lock(m_Mutex);
    FILE_CLOSE(n_hFile);
    OnCreateDirFromFilePath(m_strBaseName);
    m_strCurrentFile = generateFileName(m_iCurrentIndex);
    n_hFile = fopen(m_strCurrentFile.c_str(), "a");

    fseek(n_hFile, 0, SEEK_END);
    m_iCurrentSize = ftell(n_hFile);
}

void FileLogger::rotateFiles() {
    //std::lock_guard<std::mutex> lock(m_Mutex);
    m_iCurrentIndex++;
    m_iCurrentIndex = m_iCurrentIndex % 5000;
    openCurrentFile();
    purgeOldFiles();
}

void FileLogger::purgeOldFiles()
{
    if (m_iMaxFiles <= 0) 
        return;
    map<uint64_t,string> mapFilePath;
#ifdef _WIN32

    WIN32_FIND_DATAA findData;
    string strDir = getDirFromFilePath(m_strBaseName);
    HANDLE hFind = FindFirstFileA((m_strFilePrefix + "_*" + m_strFileExt).c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                uint64_t uTime = (static_cast<uint64_t>(findData.ftLastWriteTime.dwHighDateTime) << 32) | findData.ftLastWriteTime.dwLowDateTime;
                mapFilePath[uTime] = (strDir + findData.cFileName);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }

    size_t iIndex = mapFilePath.size();
    for (auto item : mapFilePath)
    {
        if (iIndex > (size_t)m_iMaxFiles)
        {
            if (DeleteFileA(item.second.c_str()) == 0)
                perror(("Error removing old log: " + item.second).c_str());
            iIndex--;
        }
        else
            break;
    }
#else
    string strDir = getDirFromFilePath(m_strBaseName);
    //printf("ext length:%d\n",m_ext.length());
 
  
    DIR *dir = opendir(strDir.c_str());
    if ( dir == NULL )
    {
        printf("[ERROR] %s is not a directory or not exist!", strDir.c_str());
        return;
    }

    struct dirent* d_ent = NULL;
  

    while ( (d_ent = readdir(dir)) != NULL )
    {
        if ( (strcmp(d_ent->d_name, ".") != 0) && (strcmp(d_ent->d_name, "..") != 0) )
        {
 
            if ( d_ent->d_type != DT_DIR)
            {
                string d_name(d_ent->d_name);
                //printf("%s\n",d_ent->d_name);
                if (strcmp(d_name.c_str () + d_name.length () - m_strFileExt.length(), m_strFileExt.c_str ()) == 0)
                {
                    struct stat statbuf;
                    if (stat(d_name.c_str(), &statbuf) != 0) {
                        return; // 
                    }

                    uint64_t uTime = (static_cast<uint64_t>(statbuf.st_mtim.tv_nsec));
      
                    string strAbsolutePath;
                    //string absolutePath = directory + string("/") + string(d_ent->d_name);
                    if (strDir[strDir.length()-1] == '/')
                       strAbsolutePath = strDir + string(d_ent->d_name);  
                    else
                        strAbsolutePath = strDir + "/" + string(d_ent->d_name);                    
                    mapFilePath[uTime] = strAbsolutePath;
                }
            }
        }
    }
    closedir(dir);

    size_t iIndex = mapFilePath.size();
    for (auto item : mapFilePath)
    {
        if (iIndex > (size_t)m_iMaxFiles)
        {
            if (remove(item.second.c_str()) != 0)
                perror(("Error removing old log: " + item.second).c_str());
            iIndex--;
        }
        else
            break;
    }
#endif 
}



int FileLogger::findMaxFileIndex() {
    std::vector<int> vecIndices;

#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((m_strFilePrefix + "_*" + m_strFileExt).c_str(), &findData);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                processFileName(findData.cFileName, vecIndices);
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR* dir = opendir(".");
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type == DT_REG) {
                processFileName(ent->d_name, vecIndices);
            }
        }
        closedir(dir);
    }
#endif

    return vecIndices.empty() ? 0 : *std::max_element(vecIndices.begin(), vecIndices.end());
}

void FileLogger::processFileName(const std::string& fileName, std::vector<int>& indices) 
{
#if 0
    const std::string prefix = m_strFilePrefix + "_";
    const std::string ext = m_strFileExt;

    if (fileName.size() <= prefix.size() + ext.size())
        return;
    if (fileName.substr(0, prefix.size()) != prefix)
        return;
    if (fileName.substr(fileName.size() - ext.size()) != ext) 
        return;

    const std::string numStr = fileName.substr(prefix.size(),
        fileName.size() - prefix.size() - ext.size()
    );

    if (isNumber(numStr)) {
        try {
            indices.push_back(std::stoi(numStr));
        }
        catch (...) {}
    }
#else
    size_t pos = fileName.find_last_of("_");
    if (pos != string::npos)
    {
        const std::string numStr = fileName.substr(pos + 1);
        indices.push_back(std::stoi(numStr));
    }
#endif

}

bool FileLogger::isNumber(const std::string& s) {
    return !s.empty() &&
        std::all_of(s.begin(), s.end(),
            [](unsigned char c) { return std::isdigit(c); });
}

std::string FileLogger::generateFileName(int index) const {
    return m_strFilePrefix + "_" + std::to_string(index) + m_strFileExt;
}