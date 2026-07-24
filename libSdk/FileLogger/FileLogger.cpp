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

#include "FileLogger.h"
#include <stdio.h>
//#include <map>
//#include <set>
#include <algorithm>
#include <cctype>
#include "../Common/FileSystem.h"
//#include "../Common/StringUtils.h"
#include "../Common/LockQueue.h"
#include "../mem/ConcurrentMem.h"
#include "formatPattern.h"
#include "OutPutMode.h"
#include "Common/json.hpp"


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

#define FILE_CLOSE(x)					               if((x)){if(!FileSystem::closeFile((x)))(x) = nullptr;}
#define OUT_CONSOLE                                    0x0
#define OUT_LOC_FILE                                   0x01
#define OUT_NET_UDP                                    0x02


#define LOG_SELF_LOG(Level,format, ...)                log(false,Level,__func__, __FILE__, __LINE__,format, ##__VA_ARGS__);
#define FREE_MEM(x)                                    if((x)) {PM_FREE((x)); (x)=nullptr;}
FileLogger FileLogger::m_sFileLogger;

using json = nlohmann::json;
typedef struct LOG_CONFIG_INFO
{
    int iMaxFileNum = 10;
    int iMaxFileSize = 0x3200000;//0x500000;
    int iNetPort = 9000;
    int iOutPutFile = OUT_CONSOLE;
    LogLevel emLogLevel = EM_LOG_DEBUG;
    bool bSync = false;
    std::string strLogFormat = "[%D] [%l] [tid:%t] [%!] %v%n";
    std::string strBaseName = "./Logs/log.log";
    std::string strNetIpAddr ="127.0.0.1";
}LOG_CONFIG_INFO,*LP_LOG_CONFIG_INFO;

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

// static std::map<std::string, std::string> parseConfig(const std::string& path) 
// {
//     std::map<std::string, std::string> config;
//     config.clear();
//     if (path.empty())
//         return config;

//     std::ifstream file(path);
//     if(!file.is_open())
//         return config;
//     std::string line;

//     while (std::getline(file, line))
//     {
//         std::string strData = subLeft(line, "#");
//         if (strData.empty())
//             continue;
//         strData[strcspn(strData.c_str(), "\r\n")] = 0;
//         std::pair <std::string, std::string> pairKv = spiltKv(strData);
//         if(!pairKv.first.empty())
//             config.insert(pairKv);
//     }
//     return config;
// }


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
template < typename T>
static inline auto OnGetKey2Value(const json& j, const char *strKey, T defValue = T())
{
    if(j.contains(strKey)) 
    {
        json jValue = j.at(strKey);
        if(jValue.is_primitive())
            return jValue.get<T>();
    }
    return defValue;
}

static inline void from_json(const json& jsonData, LOG_CONFIG_INFO& tagConfig) 
{
   tagConfig.strBaseName = OnGetKey2Value<std::string>(jsonData, "file_name", tagConfig.strBaseName);
   tagConfig.strLogFormat = OnGetKey2Value<std::string>(jsonData, "log_format", tagConfig.strLogFormat);
   
   tagConfig.iMaxFileNum = OnGetKey2Value<int>(jsonData, "max_files", tagConfig.iMaxFileNum); 
   tagConfig.iMaxFileSize = OnGetKey2Value<int>(jsonData, "max_size", tagConfig.iMaxFileSize); 
   tagConfig.emLogLevel = OnStringToLevel(OnGetKey2Value<std::string>(jsonData, "log_level"));
   if(jsonData.contains("out_put"))
   {
        tagConfig.iOutPutFile = OUT_CONSOLE;
        std::string strOuput;
        json jOutput = jsonData.at("out_put");
        if(jOutput.is_primitive())
        {
            strOuput = jOutput.get<std::string>();
            if (equals(strOuput.c_str(), "console", false))
                tagConfig.iOutPutFile = OUT_CONSOLE;
            else if(equals(strOuput.c_str(), "file", false))
                tagConfig.iOutPutFile = OUT_LOC_FILE;
        }  
        else if(jOutput.is_object())
        {
            tagConfig.iOutPutFile = OUT_NET_UDP;
            tagConfig.strNetIpAddr = OnGetKey2Value<std::string>(jOutput, "netIp", tagConfig.strNetIpAddr);
            tagConfig.iNetPort = OnGetKey2Value<int>(jOutput, "netPort", tagConfig.iNetPort);
        }
        else
            perror("out_put config error\n");
   }  
    tagConfig.bSync = OnGetKey2Value<bool>(jsonData, "syncMode", true);  
}

static bool OnGetLogConfig(std::ifstream &inFile,LOG_CONFIG_INFO &tagConfig)
{
    json inputData = json::parse(inFile, nullptr, false, true);
    if(inputData.is_discarded() || !inputData.contains("LogConfig"))
    {
        FileSystem::MAPSTRING mapCfg = FileSystem::parseConfig(inFile);
        if (!mapCfg.empty())
        {
            if (!mapCfg["file_name"].empty())
                tagConfig.strBaseName = mapCfg["file_name"];
        
            if(!mapCfg["max_files"].empty())
                tagConfig.iMaxFileNum = str2Int(mapCfg["max_files"]);

            if(!mapCfg["max_size"].empty())
                tagConfig.iMaxFileSize = str2Uint(mapCfg["max_size"]);
        
            if (!mapCfg["log_level"].empty())
                tagConfig.emLogLevel = OnStringToLevel(mapCfg["log_level"]);
        
            if (!mapCfg["out_put"].empty())
            {
                if (equals(mapCfg["out_put"].c_str(), "console", false))
                    tagConfig.iOutPutFile = OUT_CONSOLE;
                else if(equals(mapCfg["out_put"].c_str(), "netudp", false))
                    tagConfig.iOutPutFile = OUT_NET_UDP;
                else
                    tagConfig.iOutPutFile = OUT_LOC_FILE;
            }

            if (!mapCfg["out_mode"].empty())
                tagConfig.bSync = equals(mapCfg["out_mode"].c_str(), "sync", false);
        
            if (!mapCfg["log_pattern"].empty())
                tagConfig.strLogFormat = mapCfg["log_pattern"];

            if (tagConfig.iOutPutFile == OUT_NET_UDP)
            {
                if (!mapCfg["netIp"].empty())
                {
                    std::pair<std::string, std::string> pairNet = spiltKv(mapCfg["netIp"], ':');
                    if (!pairNet.first.empty() && !pairNet.second.empty())
                    {
                        tagConfig.strNetIpAddr = pairNet.first;
                        tagConfig.iNetPort = str2Int(pairNet.second);
                    }
                    else
                        tagConfig.iOutPutFile = OUT_LOC_FILE;
                }
                else
                    tagConfig.iOutPutFile = OUT_LOC_FILE;
            }
            return true;
        }       
    }
    else if(inputData.contains("LogConfig"))
    {
        auto jsonLogConfig = inputData.at("LogConfig");
        if(jsonLogConfig.is_object())
            tagConfig = jsonLogConfig.get<LOG_CONFIG_INFO>();
        else
            perror("LogConfig error\n");
        return true;
    }
    return false;
}

FileLogger& FileLogger::getInstance() {
    return m_sFileLogger;
}

void FileLogger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_emLogLevel = level;
    LOG_SELF_LOG(EM_LOG_INFO,"Change log level {}",(uint8_t)level);
}

void FileLogger::initLogJsonStr(const std::string& strJsonStr)
{
    closeLog();  
    if(strJsonStr.empty())
        return;
    LOG_CONFIG_INFO tagConfig; 
    json inputData = json::parse(strJsonStr, nullptr, false, true);
    if(inputData.contains("LogConfig"))
    {
        auto jsonLogConfig = inputData.at("LogConfig");
        if(jsonLogConfig.is_object())
            tagConfig = jsonLogConfig.get<LOG_CONFIG_INFO>();
        else
            perror("LogConfig error\n");
    }

    m_emLogLevel = tagConfig.emLogLevel;
    m_bSync = tagConfig.bSync;    
    m_pPatternFmt = new FormatterBuilder(tagConfig.strLogFormat);
    if (tagConfig.iOutPutFile == OUT_NET_UDP)
    {
        m_pOutputMode = new UdpOutPutMode;
        m_pOutputMode->initOutMode(tagConfig.strNetIpAddr.c_str(), tagConfig.iNetPort);
    }

    else if (tagConfig.iOutPutFile == OUT_LOC_FILE)
    {
        m_pOutputMode = new FileOutPutMode;
        std::string strBaseAbsolute = FileSystem::relative2AbsolutePath(tagConfig.strBaseName);
        m_pOutputMode->initOutMode(strBaseAbsolute.c_str(), tagConfig.iMaxFileSize);
        m_pOutputMode->setMaxFileNum(tagConfig.iMaxFileNum);
    } 
    else
        m_pOutputMode = new ConsoleOutPutMode;

    LOG_SELF_LOG(EM_LOG_INFO,"Init log finsh,outmodel: {}, filename: {}, maxsize: {}, maxnum: {}, logPattern: {}",
        tagConfig.iOutPutFile, tagConfig.strBaseName, tagConfig.iMaxFileSize, tagConfig.iMaxFileNum, tagConfig.strLogFormat);
    if (!m_bSync)
    {
        std::thread tRead(&FileLogger::outPut2File, this);
        tRead.detach();
    }
}

void FileLogger::initLog(const std::string &strCfgName)
{
    closeLog(); 
    LOG_CONFIG_INFO tagConfig;
    if (!strCfgName.empty())
    {
        std::ifstream inFile(strCfgName);
        if (!inFile.is_open()) 
        {
            std::cerr << "无法打开文件 "<< strCfgName << std::endl;
            return;
        }
        if(OnGetLogConfig(inFile, tagConfig))
        {
            m_emLogLevel = tagConfig.emLogLevel;
            m_bSync = tagConfig.bSync;
        }
        else
            printf("read config error,use default config\n");
    }
    // printf("read config info {outmodel: %d, filename: %s, maxsize: %d, maxnum: %d, logPattern: %s}\n",
    //     tagConfig.iOutPutFile, tagConfig.strBaseName.c_str(), tagConfig.iMaxFileSize, tagConfig.iMaxFileNum, tagConfig.strLogFormat.c_str());    
    m_pPatternFmt = new FormatterBuilder(tagConfig.strLogFormat);
    if (tagConfig.iOutPutFile == OUT_NET_UDP)
    {
        m_pOutputMode = new UdpOutPutMode;
        m_pOutputMode->initOutMode(tagConfig.strNetIpAddr.c_str(), tagConfig.iNetPort);
    }

    else if (tagConfig.iOutPutFile == OUT_LOC_FILE)
    {
        m_pOutputMode = new FileOutPutMode;
        std::string strBaseAbsolute = FileSystem::relative2AbsolutePath(tagConfig.strBaseName);
        m_pOutputMode->initOutMode(strBaseAbsolute.c_str(), tagConfig.iMaxFileSize);
        m_pOutputMode->setMaxFileNum(tagConfig.iMaxFileNum);
    } 
    else
        m_pOutputMode = new ConsoleOutPutMode;

    LOG_SELF_LOG(EM_LOG_INFO,"Init log finsh,outmodel: {}, filename: {}, maxsize: {}, maxnum: {}, logPattern: {}",
        tagConfig.iOutPutFile, tagConfig.strBaseName, tagConfig.iMaxFileSize, tagConfig.iMaxFileNum, tagConfig.strLogFormat);
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
        LOG_SELF_LOG(EM_LOG_INFO,"Change log file name or path {}",strFileName);
    }
}

void FileLogger::closeLog()
{
    if (m_pOutputMode)
    {
        LOG_SELF_LOG(EM_LOG_INFO,"Close log");
        m_pOutputMode->closeOutPut();

        delete m_pOutputMode;
        m_pOutputMode = nullptr;
    }
    if(m_pPatternFmt)
    {
        delete m_pPatternFmt;
        m_pPatternFmt = nullptr;
    }
}

FileLogger::FileLogger(LogLevel level)
    :m_emLogLevel(level),
    m_bSync(false),
    m_pOutputMode(nullptr),
    m_pPatternFmt(nullptr)
    
{
    // m_pOutputMode = new FileOutPutMode;
    // m_pOutputMode->initOutMode("./logs/logfile.log", 0x500000);
    // m_pOutputMode->setMaxFileNum(10);
    m_ctxQueue.setSpaces(200);
}

FileLogger::~FileLogger() 
{
    closeLog();
}

void FileLogger::formatMessage(LogLevel emLevel, const char* szFunName, const char* szFileName, 
    const int iLine, const std::string & strMessage)
{
    if(!m_pPatternFmt)
        initLog("");
    // //{
    //     //std::cout << "Logger not init->formatMessage" <<std::endl;
    //     //return;
    //     initLog("");
    // //}
    LogMessage logMsg(emLevel, szFileName, iLine, strMessage.c_str(), szFunName);
    memory_buf_t bufDest;
    std::string strFormatted = m_pPatternFmt->format(logMsg);
    writeToOutPut(emLevel, strFormatted);
}

void FileLogger::outPut2File()
{
    while (1)
    {     
        do 
        {
            std::pair<std::string,int> pairData = m_ctxQueue.pop_front();
            m_pOutputMode->writeData(pairData.first,pairData.second); 
        }while(!m_ctxQueue.empty());
        m_pOutputMode->flushFile();
    }
}

void FileLogger::writeToOutPut(LogLevel &emLevel, const std::string& strMsg)
{
    if(!m_pOutputMode)
    {
        std::cout << "Logger not init->formatMessage" <<std::endl;
        return;
    }

    if (!m_bSync)
        m_ctxQueue.push(std::pair<std::string,int>(strMsg,emLevel));
    else
    {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_pOutputMode->writeData(strMsg, emLevel);
        }

        //m_pOutputMode->flushFile();
    }
}
