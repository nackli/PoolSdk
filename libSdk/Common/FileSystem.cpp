/*
Written by Nack li <nackli@163.com>
Copyright (c) 2024. All Rights Reserved.
*/

#include "FileSystem.h"
#include <string>
#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif
#include <algorithm>
#include <cstring>
#include <iostream>

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

vector<string> getFilesInDirectory(const string& strFileDir, const char *szExt)
{
	if (strFileDir.empty())
		return std::vector<string>();

	std::vector<string> files;
#ifdef _WIN32	
	WIN32_FIND_DATAA findData;
	string strDir = strFileDir;
	if (strDir[strDir.length() - 1] != '/' && strDir[strDir.length() - 1] != '\\')
		strDir += "\\";

	strDir = strDir + "*" + szExt;
	HANDLE hFind = FindFirstFileA(strDir.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE) {
		//LOG_ERROR("Open dir fiall: %d", GetLastError());
		return std::vector<string>();
	}
	do {
		if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			string strFileName = findData.cFileName;
			if (all_of(strFileName.begin(), strFileName.end() - strlen(szExt), [] (const char cChar){
				if(isdigit(cChar)  || cChar =='_') 
					return true; 
				return false;}))
			{
				string strDir = strFileDir;
				string strPath;
				if (strDir[strDir.length() - 1] == '/' ||
					strDir[strDir.length() - 1] == '\\')
					strPath = strDir + findData.cFileName;
				else
					strPath = strFileDir + "\\" + findData.cFileName;
				files.emplace_back(strPath);
			}
		}
	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);
	std::sort(files.begin(), files.end(),
		[](const string& strLeft, const string& strRight) {return strLeft < strRight; });
#else
    //printf("ext length:%d\n",m_ext.length());
 

    DIR *dir = opendir(strFileDir.c_str());
    if ( dir == NULL )
    {
        printf("[ERROR] %s is not a directory or not exist!", strFileDir.c_str());
        return std::vector<string>();
    }
 
    struct dirent* d_ent = NULL;
    while ( (d_ent = readdir(dir)) != NULL )
    {

        if ( (strcmp(d_ent->d_name, "..") != 0) && (strcmp(d_ent->d_name, ".") != 0) )
        {

            if ( d_ent->d_type != DT_DIR)
            {
                string d_name(d_ent->d_name);
                //printf("%s\n",d_ent->d_name);
                if (strcmp(d_name.c_str () + d_name.length () - strlen(szExt), szExt) == 0)
                {
					string strAbsolutePath;
      				if (strFileDir[strFileDir.length()-1] == '/')
                       strAbsolutePath = strFileDir + string(d_ent->d_name);  
                    else
                        strAbsolutePath = strFileDir + "/" + string(d_ent->d_name);        
                    files.emplace_back(strAbsolutePath);
                }
            }
        }
    }
    // sort the returned files
    sort(files.begin(), files.end());
 
    closedir(dir);		
#endif		
	return 	std::vector<string>(files.begin(), files.end());
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
    std::string strSubPath;
    size_t iPos = 0;


    while ((iPos = path.find('/', iPos)) != std::string::npos) 
    {
        strSubPath = path.substr(0, iPos++);

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

void createDirFromFilePath(const string &strFilePath)
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

bool delFile(const string& strFilePath)
{
	if (strFilePath.empty())
		return false;
	return ::DeleteFileA(strFilePath.c_str());
}
#endif

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

string relative2AbsolutePath(const std::string& strRelaPath)
{
#ifndef _WIN32
	const uint16_t MAX_PATH = 256;
#endif
	char szFullPath[MAX_PATH];
#ifdef _WIN32
	GetFullPathNameA(strRelaPath.c_str(), MAX_PATH, szFullPath, nullptr);
	return std::string(szFullPath);
#else
	if(!realpath(strRelaPath.c_str(), szFullPath))
		std::cerr << "real path fun: " << strerror(errno) << std::endl;
	return std::string(szFullPath);
#endif
}

