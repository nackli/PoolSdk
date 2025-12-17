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

#include "FileSystem.h"
#include <string>
#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <cstdint>
#include <unistd.h>
#endif
#include <algorithm>
#include <cstring>
#include <iostream>
#include <vector>
#include "StringUtils.h"
#include <fstream>
using namespace std;
namespace FileSystem
{
#ifdef _WIN32	
	std::string DosPathToNtPath(const std::string& strPath)
	{
		std::string strResultPath;
		char szDriveStrings[MAX_PATH] = { 0 };
		char szDosBuf[MAX_PATH] = { 0 };
		LPSTR pDriveStr = NULL;

		do
		{
			if (!::GetLogicalDriveStringsA(_countof(szDriveStrings), szDriveStrings))
				break;

			for (int i = 0; i < _countof(szDriveStrings); i += 4)
			{
				pDriveStr = &szDriveStrings[i];
				pDriveStr[2] = ('\0');

				DWORD dwCch = ::QueryDosDeviceA(pDriveStr, szDosBuf, _countof(szDosBuf));
				if (!dwCch)
					break;


				if (dwCch >= 2)
					dwCch -= 2;

				if (strPath.size() < dwCch)
					break;


				if (('\\') == strPath[dwCch] && 0 == strncmp(strPath.c_str(), szDosBuf, dwCch))
				{
					strResultPath = pDriveStr;
					strResultPath += &strPath[dwCch];
					break;
				}
			}

		} while (false);
		return strResultPath;
	}

	std::string NtPathToDosPath(const std::string& strPath)
	{
		std::string strResultPath;

		char szDosBuf[MAX_PATH] = { 0 };

		do
		{
			if (strPath.size() < 2)
				break;


			if ((':') != strPath[1] || ('\\') == strPath[0])
				break;


			if (!::QueryDosDeviceA(strPath.substr(0, 2).c_str(), szDosBuf, _countof(szDosBuf)))
				break;

			strResultPath = szDosBuf;
			strResultPath += strPath.substr(2);

		} while (false);

		return strResultPath;
	}
#endif

	static inline string OnBuildFindFileReg(const string& strPre, const string& strExt)
	{
		string strReg = strPre;
		if (FileSystem::IsDirectoryExists(strReg))
		{
			if (strReg[strReg.length() - 1] != '/' && strReg[strReg.length() - 1] != '\\')
				strReg += "\\";	
		}
		if(!strExt.empty())
			return strReg = strReg + "*" + strExt;
		return strReg;
	}

	FileInfoList getFilesInCurDir(const string& strFilePathAndReg, const string& strExt,bool bOnlyFileName)
	{
		if (strFilePathAndReg.empty())
			return FileInfoList();

		string strFind = OnBuildFindFileReg(strFilePathAndReg, strExt);
		FileInfoList files;
		string strDir = getDirFromFilePath(strFind);
#ifdef _WIN32	
		WIN32_FIND_DATAA findData;
		
		HANDLE hFind = FindFirstFileA(strFind.c_str(), &findData);

		if (hFind == INVALID_HANDLE_VALUE) {
			//LOG_ERROR("Open dir fiall: %d", GetLastError());
			return FileInfoList();
		}
		do {
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				FILEINFO tagFileInfo;
				tagFileInfo.lastWriteTime = findData.ftLastWriteTime;
				if (!bOnlyFileName)
				{
					if (strDir[strDir.length() - 1] == '/' ||
						strDir[strDir.length() - 1] == '\\')
						tagFileInfo.strFileName = strDir + findData.cFileName;
					else
						tagFileInfo.strFileName = strDir + "\\" + findData.cFileName;
				}
				else
					tagFileInfo.strFileName = findData.cFileName;

				files.emplace_back(tagFileInfo);
			}
		} while (FindNextFileA(hFind, &findData));

		FindClose(hFind);
		files.sort([](const FILEINFO& tagLeft, const FILEINFO& tagRight) {return CompareFileTime(&tagLeft.lastWriteTime, &tagRight.lastWriteTime) < 0; });
#else
		DIR* dir = opendir(strDir.c_str());
		if (dir == NULL)
		{
			printf("[ERROR] %s is not a directory or not exist!\n", strDir.c_str());
			return FileInfoList();
		}

		struct dirent* d_ent = NULL;
		while ((d_ent = readdir(dir)) != NULL)
		{

			if ((strcmp(d_ent->d_name, "..") != 0) && (strcmp(d_ent->d_name, ".") != 0))
			{

				if (d_ent->d_type != DT_DIR)
				{
					string strFileName(d_ent->d_name);
					if (strExt.compare(strFileName.c_str() + strFileName.length() - strExt.length()) == 0)
					{
						FILEINFO tagFileInfo;
						if (strDir[strDir.length() - 1] == '/')
							tagFileInfo.strFileName = strDir + string(d_ent->d_name);
						else
							tagFileInfo.strFileName = strDir + "/" + string(d_ent->d_name);
						struct stat fileStat;
						if (stat(tagFileInfo.strFileName.c_str(), &fileStat) == 0)
							tagFileInfo.lastWriteTime  = static_cast<long long>(fileStat.st_mtime) * 1000 + fileStat.st_mtim.tv_nsec / 1000000;
			
						files.emplace_back(tagFileInfo);
					}
				}
			}
		}
		// sort the returned files
		files.sort([](const FILEINFO& tagLeft, const FILEINFO& tagRight) {return tagLeft.lastWriteTime < tagRight.lastWriteTime; });
		closedir(dir);
#endif		
		return 	files;
	}

	bool IsDirectoryExists(const std::string& strDir)
	{
		if (strDir.empty())
			return false;
#ifdef _WIN32     
		DWORD attrib = GetFileAttributesA(strDir.c_str());
		return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
#else
		struct stat statbuf;
		if (stat(strDir.c_str(), &statbuf) != 0) {
			return false;
		}
		return S_ISDIR(statbuf.st_mode);
#endif    
	}

	bool IsFileExists(const string& strFilePath)
	{
		if (strFilePath.empty())
			return false;

#ifdef _WIN32     
		DWORD attrib = GetFileAttributesA(strFilePath.c_str());
		return attrib != INVALID_FILE_ATTRIBUTES;
#else
		struct stat statbuf;
		if (stat(strFilePath.c_str(), &statbuf) != 0) {
			return false;
		}
		return S_ISREG(statbuf.st_mode);
#endif   		
	}


	std::string getDirFromFilePath(const std::string& filepath) {
		size_t pos = filepath.find_last_of("/\\");
		if (pos != std::string::npos)
			return filepath.substr(0, pos + 1);
		return "";
	}


	static bool OnCreateDirectoryRecursive(std::string& path)
	{
#ifdef _WIN32    
		if (!path.empty() && (path.back() == '\\' || path.back() == '/'))
			path.pop_back();

		if (CreateDirectoryA(path.c_str(), nullptr))
			return true;

		DWORD error = GetLastError();
		if (error == ERROR_ALREADY_EXISTS)
		{
			DWORD attrib = GetFileAttributesA(path.c_str());
			return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
		}
		else if (error == ERROR_PATH_NOT_FOUND)
		{
			std::string parentPath = getDirFromFilePath(path);
			if (parentPath.empty())
				return false;


			if (OnCreateDirectoryRecursive(parentPath))
				return CreateDirectoryA(path.c_str(), nullptr);
		}
		return false;
#else
		size_t iPos = 0;
		if (path.empty())
			return true;

		while ((iPos = path.find('/', iPos)) != std::string::npos)
		{
			std::string strSubPath = path.substr(0, iPos++);

			if (strSubPath.empty())
				continue;

			if (mkdir(strSubPath.c_str(), 0755) && errno != EEXIST)
			{
				std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
				return false;
			}
		}

		if (mkdir(path.c_str(), 0755) && errno != EEXIST)
		{
			std::cerr << "Error creating directory: " << strerror(errno) << std::endl;
			return false;
		}
		return true;
#endif   
	}



	bool createDirectoryRecursive(std::string& strDir)
	{
#if 0	
		if (!strDir.empty() && (strDir.back() == '\\' || strDir.back() == '/'))
			strDir.pop_back();

		if (CreateDirectoryA(strDir.c_str(), nullptr))
			return true;

		DWORD error = GetLastError();
		if (error == ERROR_ALREADY_EXISTS)
		{
			DWORD attrib = GetFileAttributesA(strDir.c_str());
			return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
		}
		else if (error == ERROR_PATH_NOT_FOUND)
		{
			std::string strParentPath = getDirectory(strDir);
			if (strParentPath.empty())
				return false;

			if (createDirectoryRecursive(strParentPath))
				return CreateDirectoryA(strDir.c_str(), nullptr);
		}
		return false;
#else
		return OnCreateDirectoryRecursive(strDir);
#endif	
	}

	string getDirectory(const string& strFilePath)
	{
		if (strFilePath.empty())
			return string();
		size_t pos = strFilePath.find_last_of("/\\");
		if (pos != std::string::npos)
			return strFilePath.substr(0, pos + 1);
		return "";
	}

	string getFileName(const string& strFilePath)
	{
		if (strFilePath.empty())
			return string();
		size_t pos = strFilePath.find_last_of("/\\");
		if (pos != std::string::npos)
			return strFilePath.substr(pos + 1);
		return "";
	}

	void createDirFromFilePath(const string& strFilePath)
	{
		string strDirName = getDirectory(strFilePath);
		if (!IsDirectoryExists(strDirName))
			createDirectoryRecursive(strDirName);
	}
#ifdef _WIN32
	HANDLE openOrCreateFile(const string& strFilePath, uint32_t uDesiredAccess, uint32_t uCreationDisposition,
		uint32_t uFlagsAndAttributes, uint32_t uShartMode)
	{
		createDirFromFilePath(strFilePath);
		return CreateFileA(strFilePath.c_str(), uDesiredAccess, uShartMode, NULL,
			uCreationDisposition, uFlagsAndAttributes, NULL);
	}

	bool setFilePoint(HANDLE hFile, uint32_t uPoint, uint8_t uMoveMethod)
	{
		if (hFile == INVALID_HANDLE_VALUE)
			return false;
		SetFilePointer(hFile, uPoint, 0, uMoveMethod);
		return true;
	}

	bool writeFile(HANDLE hFile, void* pData, uint32_t uSize)
	{
		if (hFile == INVALID_HANDLE_VALUE)
			return false;
		return WriteFile(hFile, pData, uSize, nullptr, nullptr);
	}

	bool flushFile(HANDLE hFile)
	{
		if (hFile == INVALID_HANDLE_VALUE)
			return false;
		return FlushFileBuffers(hFile);
	}

	bool closeFile(HANDLE hFile)
	{
		if (hFile == INVALID_HANDLE_VALUE)
			return false;
		return CloseHandle(hFile);
	}

	bool readFile(HANDLE hFile, void* pData, uint32_t uSize)
	{
		if (hFile == INVALID_HANDLE_VALUE || !pData || !uSize)
			return false;
		if (!ReadFile(hFile, pData, uSize, nullptr, nullptr))
			return false;
		return true;
	}

	size_t getFileSize(HANDLE hFile)
	{
		if (hFile == INVALID_HANDLE_VALUE)
			return false;
		int fileSize = GetFileSize(hFile, NULL);
		return fileSize;
	}

	bool winDelFile(const string& strFilePath)
	{
		if (strFilePath.empty())
			return false;
		return ::DeleteFileA(strFilePath.c_str());
	}


	bool winMoveFile(const string& strOldFilePath, const string& strNewFilePath)
	{
		if (strOldFilePath.empty() || strNewFilePath.empty())
			return false;
		return ::MoveFileA(strOldFilePath.c_str(), strNewFilePath.c_str());
	}
#endif

	FILE* openOrCreateFile(const string& strFilePath, const char* szFlags)
	{
		if (strFilePath.empty())
			return nullptr;
		createDirFromFilePath(strFilePath);
		return fopen(strFilePath.c_str(), szFlags);
	}

	FILE* openOrCreateFile(const char* szFilePath, const char* szFlags)
	{
		if (szFilePath && szFilePath[0])
		{
			createDirFromFilePath(szFilePath);
			return fopen(szFilePath, szFlags);
		}
		return nullptr;
	}

	FILE* reopenOrCreateFile(const char* szFilePath, FILE* hFile, const char* szFlags)
	{
		if (szFilePath && szFilePath[0])
		{
			if (!hFile)
				return openOrCreateFile(szFilePath, szFlags);
			else
			{
				createDirFromFilePath(szFilePath);
				return freopen(szFilePath, szFlags, hFile);
			}
			
		}
		return nullptr;
	}

	bool writeFile(FILE* hFile, void* pData, uint32_t uSize)
	{
		if (!hFile || !pData || !uSize)
			return false;
		return fwrite(pData, uSize, 1, hFile) > 0;
	}

	bool readFile(FILE* hFile, void* pData, uint32_t uSize)
	{
		if (!hFile || !pData || !uSize)
			return false;
		return fread(pData, uSize, 1, hFile) > 0;
	}

	bool flushFile(FILE* hFile,bool bForcedDisk)
	{
		if (!hFile)
			return false;
	
		if(fflush(hFile) == 0)
		{
#ifndef _MSC_VER	
			if(bForcedDisk)
			{
				int fd = ::fileno(hFile);
				::fsync(fd);
			}
#endif	
			return true;
		}
		return false;
	}

	bool fseekFile(FILE* hFile, int iPos, int origin)
	{
		if (!hFile)
			return false;
		return fseek(hFile, iPos, origin) == 0;
	}

	long getFileCurPos(FILE* hFile)
	{
		if (!hFile)
			return 0;
		return ftell(hFile);
	}

	size_t getFileSize(FILE* hFile)
	{
		fseekFile(hFile, 0, SEEK_END);
		return getFileCurPos(hFile);
	}

	bool fgetsFile(FILE* hFile, char* szData, size_t uSize)
	{
		if (!hFile || !szData || !uSize)
			return false;
		return fgets(szData, uSize, hFile) != nullptr;
	}

	void rewindFile(FILE* hFile)
	{
		if (!hFile)
			return;
		rewind(hFile);
	}

	bool IsEofFile(FILE* hFile)
	{
		if (!hFile)
			return true;
		return feof(hFile) != 0;
	}

	bool closeFile(FILE* hFile)
	{
		if (!hFile)
			return false;
		return fclose(hFile) == 0;
	}

	bool moveFile(const char* szOldFile, const char* szNewFile)
	{
		if (!szOldFile || !szNewFile)
			return false;
		if (rename(szOldFile, szNewFile) == 0)
			return true;
		return false;
	}

	bool delFile(const char* szFilePath)
	{
		if (!szFilePath)
			return false;
		if (remove(szFilePath) == 0)
			return true;
		return false;
	}

	bool IsAbsolutePath(const std::string& strPath)
	{
#ifdef _WIN32
		if (strPath.size() >= 3 && strPath[1] == ':' && (strPath[2] == '/' || strPath[2] == '\\'))
			return true;
		if (strPath.size() >= 2 && (strPath[0] == '\\' && strPath[1] == '\\') || (strPath[0] == '/' && strPath[1] == '/'))
			return true;
		return false;
#else
		return !strPath.empty() && strPath[0] == '/';
#endif
	}

	// 规范化路径，处理 . 和 .. 以及反斜杠
inline std::string normalizePath(const std::string& path) {
    std::string normalized;
    std::vector<std::string> components;
    std::string current;
    
    // 首先替换所有反斜杠为正斜杠
    std::string temp = path;
    std::replace(temp.begin(), temp.end(), '\\', '/');
    
    // 处理路径组件
    for (char c : temp)
	{
        if (c == '/') 
		{
            if (!current.empty()) 
			{
                if (current == ".")
				{

                } 
				else if (current == "..") 
				{
                    if (!components.empty()) 
                        components.pop_back();
                } 
				else 
                    components.push_back(current);
                current.clear();
            }
        } 
		else 
            current += c;
    }
    
    if (!current.empty()) {
        if (current == ".") 
		{
        } 
		else if (current == "..") 
		{
            if (!components.empty()) 
                components.pop_back();
        } 
		else 
            components.push_back(current);
    }
    
    if (temp.find('/') == 0) 
        normalized = "/"; 
    
    for (size_t i = 0; i < components.size(); ++i) 
	{
        normalized += components[i];
        if (i < components.size() - 1) 
            normalized += "/";
    }
    
    return normalized;
}

	string relative2AbsolutePath(const std::string& strRelaPath)
	{
#ifndef _WIN32
		const uint16_t MAX_PATH = PATH_MAX;
#endif
		char szFullPath[MAX_PATH];
#ifdef _WIN32
		GetFullPathNameA(strRelaPath.c_str(), MAX_PATH, szFullPath, nullptr);
		return std::string(szFullPath);
#else
#if 0
		if (!realpath(strRelaPath.c_str(), szFullPath))
			std::cerr << "real path fun: " <<strRelaPath <<" "<< strerror(errno) << std::endl;
		return std::string(szFullPath);
#else
		if (getcwd(szFullPath, sizeof(szFullPath)) != nullptr) 
		{
			return normalizePath(std::string(szFullPath) + "/" + strRelaPath);	
		}
		return 	strRelaPath;
#endif
#endif
	}


	MAPSTRING parseConfig(const std::string& path)
	{
		std::map<std::string, std::string> config;
		config.clear();
		if (path.empty())
			return config;

		std::ifstream file(path);
		if(!file.is_open())
			return config;
		std::string line;

		while (std::getline(file, line))
		{
			std::string strData = subLeft(line, "#");
			if (strData.empty())
				continue;
			strData[strcspn(strData.c_str(), "\r\n")] = 0;
			std::pair <std::string, std::string> pairKv = spiltKv(strData);
			if(!pairKv.first.empty())
				config.insert(pairKv);
		}
		return config;
	}
}
