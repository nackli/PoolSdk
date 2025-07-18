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
#ifdef _WIN32
#define UNUSED_FUN
#else
#define UNUSED_FUN __attribute__((unused))
#endif
#define FILE_CLOSE(x)					if((x)){if(!fclose((x)))(x) = nullptr;}
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

// static std::string OnGetDirectory(const std::string& filepath) {
//     size_t pos = filepath.find_last_of("/\\");
//     if (pos != std::string::npos)
//         return filepath.substr(0, pos + 1);
//     return "";
// }
// �ݹ鴴��Ŀ¼
static bool OnCreateDirectoryRecursive(std::string& path)
{
// #ifdef _WIN32    
//     if (!path.empty() && (path.back() == '\\' || path.back() == '/'))
//         path.pop_back();

//     if (CreateDirectoryA(path.c_str(), nullptr))
//         return true;

//     DWORD error = GetLastError();
//     // Ŀ¼�Ѵ��ڣ�����Ƿ�Ϊ�ļ���
//     if (error == ERROR_ALREADY_EXISTS)
//     {
//         DWORD attrib = GetFileAttributesA(path.c_str());
//         return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
//     }
//     // ·�������ڣ�������Ŀ¼
//     else if (error == ERROR_PATH_NOT_FOUND)
//     {
//         std::string parentPath = OnGetDirectory(path);
//         if (parentPath.empty())
//             return false; // ��·��Ϊ�գ����Ŀ¼��

//         // �ݹ鴴����Ŀ¼������
//         if (OnCreateDirectoryRecursive(parentPath))
//             return CreateDirectoryA(path.c_str(), nullptr);
//     }
//     return false;
//  #else
//     std::string strSubPath;
//     size_t iPos = 0;

//     // 处理路径中的每一层
//     while ((iPos = path.find('/', iPos)) != std::string::npos) 
//     {
//         strSubPath = path.substr(0, iPos++);

//         if (strSubPath.empty())
//             continue; // 忽略根路径 "/"

//         // 检查目录是否已经存在
//         if (mkdir(strSubPath.c_str(), 0755) && errno != EEXIST) 
//         {
//             std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
//             return false;
//         }
//     }

//     // 创建最后一级目录
//     if (mkdir(path.c_str(), 0755) && errno != EEXIST)
//     {
//         std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
//         return false;
//     }
//     return true;
//  #endif   
    return createDirectoryRecursive(path);
}

// ���Ŀ¼�Ƿ����
static bool OnIsDirectoryExists(const std::string& path) {
//  #ifdef _WIN32     
//     DWORD attrib = GetFileAttributesA(path.c_str());
//     return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
// #else
//     struct stat statbuf;
//     if (stat(path.c_str(), &statbuf) != 0) {
//         return false; // stat调用失败
//     }
//     return S_ISDIR(statbuf.st_mode);
// #endif    
    return IsDirectoryExists(path);
}

static void OnCreateDirFromFilePath(const string strFilePath)
{
    string strCfgDir = getDirFromFilePath(strFilePath);
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
    std::transform(strLev.begin(), strLev.end(), strLev.begin(), [](unsigned char c){ return std::toupper(c); });
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
    m_iCurrentIndex(0) 
{
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

    int neededSize = vsnprintf(nullptr, 0, format, args) + 1;

    if (neededSize <= 0) {
        va_end(args);
        throw std::runtime_error("Error formatting log message");
    }

    // ������������ʵ�ʸ�ʽ��
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
        openCurrentFile();  

    if (m_iCurrentSize + iMsgSize > m_iMaxSize)
        rotateFiles();

    if (n_hFile)
    {
        fwrite(strMsg.c_str(), strMsg.length(), 1, n_hFile);
        fflush(n_hFile);
        m_iCurrentSize += iMsgSize;
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
 
    // 打开目录, DIR是类似目录句柄的东西
    DIR *dir = opendir(strDir.c_str());
    if ( dir == NULL )
    {
        printf("[ERROR] %s is not a directory or not exist!", strDir.c_str());
        return;
    }
 
    // dirent会存储文件的各种属性
    struct dirent* d_ent = NULL;
  
    // 一行一行的读目录下的东西,这个东西的属性放到dirent的变量中
    while ( (d_ent = readdir(dir)) != NULL )
    {
        // 忽略 "." 和 ".."
        if ( (strcmp(d_ent->d_name, ".") != 0) && (strcmp(d_ent->d_name, "..") != 0) )
        {
            // d_type可以看到当前的东西的类型,DT_DIR代表当前都到的是目录,在usr/include/dirent.h中定义的
            if ( d_ent->d_type != DT_DIR)
            {
                string d_name(d_ent->d_name);
                //printf("%s\n",d_ent->d_name);
                if (strcmp(d_name.c_str () + d_name.length () - m_strFileExt.length(), m_strFileExt.c_str ()) == 0)
                {
                    struct stat statbuf;
                    if (stat(d_name.c_str(), &statbuf) != 0) {
                        return; // stat调用失败
                    }

                    uint64_t uTime = (static_cast<uint64_t>(statbuf.st_mtim.tv_nsec));
                    // 构建绝对路径
                    string strAbsolutePath;
                    //string absolutePath = directory + string("/") + string(d_ent->d_name);
                    // 如果传入的目录最后是/--> 例如"a/b/", 那么后面直接链接文件名
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