/*
Written by Nack li <nackli@163.com>
Copyright (c) 2024. All Rights Reserved.
*/

#include "FileLogger.h"
#include <stdio.h>
#include <map>
#include <set>
#define FILE_CLOSE(x)					if((x)){if(!fclose((x)))(x) = nullptr;}
FileLogger FileLogger::m_sFileLogger;

static void memory_dump(const void* ptr, unsigned int len)
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

static std::string OnGetDirectory(const std::string& filepath) {
    size_t pos = filepath.find_last_of("/\\");
    if (pos != std::string::npos)
        return filepath.substr(0, pos + 1);
    return "";
}
// 递归创建目录
static bool OnCreateDirectoryRecursive(std::string& path)
{
    if (!path.empty() && (path.back() == '\\' || path.back() == '/'))
        path.pop_back();

    if (CreateDirectoryA(path.c_str(), nullptr))
        return true;

    DWORD error = GetLastError();
    // 目录已存在，检查是否为文件夹
    if (error == ERROR_ALREADY_EXISTS)
    {
        DWORD attrib = GetFileAttributesA(path.c_str());
        return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
    }
    // 路径不存在，创建父目录
    else if (error == ERROR_PATH_NOT_FOUND)
    {
        std::string parentPath = OnGetDirectory(path);
        if (parentPath.empty())
            return false; // 父路径为空（如根目录）

        // 递归创建父目录并重试
        if (OnCreateDirectoryRecursive(parentPath))
            return CreateDirectoryA(path.c_str(), nullptr);
    }
    return false;
}

// 检查目录是否存在
static bool OnIsDirectoryExists(const std::string& path) {
    DWORD attrib = GetFileAttributesA(path.c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
}

static void OnCreateDirFromFilePath(const string strFilePath)
{
    string strCfgDir = OnGetDirectory(strFilePath);
    if (!OnIsDirectoryExists(strCfgDir))
        OnCreateDirectoryRecursive(strCfgDir);
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
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            config[key] = value;
        }
    }
    return config;
}

static LogLevel stringToLevel(const std::string& strLevel) {
    static std::map<std::string, LogLevel> levelMap = {
        {"TRACE", LogLevel::EM_LOG_TRACE},
        {"DEBUG", LogLevel::EM_LOG_DEBUG},
        {"INFO", LogLevel::EM_LOG_INFO},
        {"WARNING", LogLevel::EM_LOG_WARNING},
        {"ERROR", LogLevel::EM_LOG_ERROR},
        {"FATAL", LogLevel::EM_LOG_FATAL}
    };
    string strLev = strLevel;
    std::transform(strLev.begin(), strLev.end(), strLev.begin(), toupper);
    return levelMap.at(strLev);
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
                m_iMaxFiles = std::stoi(mapCfg["max_files"]);
            if(!mapCfg["max_size"].empty())
                m_iMaxSize = std::stoi(mapCfg["max_size"]);
            if (!mapCfg["log_level"].empty())
                m_emLogLevel = stringToLevel(mapCfg["log_level"]);
        }
        parseFileNameComponents();
        m_iCurrentIndex = findMaxFileIndex();
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

FileLogger::FileLogger(const string& strBase, size_t maxSize, int maxFiles, LogLevel level)
    : m_strBaseName(strBase),
    n_hFile(nullptr),
    m_iMaxSize(maxSize),
    m_iMaxFiles(maxFiles),
    m_iCurrentSize(0),
    m_emLogLevel(level),
    m_iCurrentIndex(0) {
}

FileLogger::~FileLogger() 
{
    //if (m_fileStream.is_open()) 
    //    m_fileStream.close();
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

std::string FileLogger::stringFormat(const char* format, ...) {
    va_list args;
    va_start(args, format);

    // 确定需要的缓冲区大小
    va_list argsCopy;
    va_copy(argsCopy, args);
    int neededSize = vsnprintf(nullptr, 0, format, argsCopy) + 1;
    va_end(argsCopy);

    if (neededSize <= 0) {
        va_end(args);
        throw std::runtime_error("Error formatting log message");
    }

    // 创建缓冲区并实际格式化
    std::unique_ptr<char[]> buf(new char[neededSize]);
    int actualSize = vsnprintf(buf.get(), neededSize, format, args);
    va_end(args);

    if (actualSize < 0) 
        throw std::runtime_error("Error formatting log message");
 

    return std::string(buf.get(), buf.get() + actualSize);
}

void FileLogger::writeToFile(const std::string& strMsg) 
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    size_t iMsgSize = strMsg.size();
        
    if (n_hFile == nullptr)
    {
        openCurrentFile();
    }         

    if (m_iCurrentSize + iMsgSize > m_iMaxSize)
    {
  
        rotateFiles();
    }

    if (n_hFile)
    {
        fwrite(strMsg.c_str(), strMsg.length(), 1, n_hFile);
        fflush(n_hFile);
        m_iCurrentSize += iMsgSize;
    }
    //m_fileStream << strMsg;

    
//    m_fileStream.flush();
}

void FileLogger::openCurrentFile() {
    //std::lock_guard<std::mutex> lock(m_Mutex);
    FILE_CLOSE(n_hFile);
    OnCreateDirFromFilePath(m_strBaseName);
    m_strCurrentFile = generateFileName(m_iCurrentIndex);
    n_hFile = fopen(m_strCurrentFile.c_str(), "w");

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

void FileLogger::purgeOldFiles() {
    if (m_iMaxFiles <= 0) return;

    map<uint64_t,string> mapFilePath;
    WIN32_FIND_DATAA findData;
    string strDir = OnGetDirectory(m_strBaseName);
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
                processFileName(ent->d_name, indices);
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