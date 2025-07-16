/*
Written by Nack li <nackli@163.com>
Copyright (c) 2024. All Rights Reserved.
*/

#include "FileSystem.h"
#include <string>
#include <Windows.h>
#include <algorithm>

FileSystem::FileSystem() {
}

FileSystem::~FileSystem() {
}

std::string FileSystem::DosPathToNtPath(const std::string& strPath)
{
	std::string strResultPath;
	char szDriveStrings[MAX_PATH] = { 0 };
	char szDosBuf[MAX_PATH] = { 0 };
	LPSTR pDriveStr = NULL;

	do
	{
		// 获取盘符名到缓冲
		if (!::GetLogicalDriveStringsA(_countof(szDriveStrings), szDriveStrings))
			break;

		// 遍历盘符名
		for (int i = 0; i < _countof(szDriveStrings); i += 4)
		{
			pDriveStr = &szDriveStrings[i];
			pDriveStr[2] = ('\0');

			// 查询盘符对应的DOS设备名称
			DWORD dwCch = ::QueryDosDeviceA(pDriveStr, szDosBuf, _countof(szDosBuf));
			if (!dwCch)
				break;

			// 结尾有 2 个 NULL, 减去 2 获得字符长度
			if (dwCch >= 2)
				dwCch -= 2;

			if (strPath.size() < dwCch)
				break;

			// 路径拼接
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

std::string FileSystem::NtPathToDosPath(const std::string& strPath)
{
	std::string strResultPath;
	char szDosBuf[MAX_PATH] = { 0 };

	do
	{
		if (strPath.size() < 2)
			break;

		// 非 NT 路径则不处理
		if ((':') != strPath[1] || ('\\') == strPath[0])
			break;

		// 查询盘符对应的DOS设备名称
		if (!::QueryDosDeviceA(strPath.substr(0, 2).c_str(), szDosBuf, _countof(szDosBuf)))
			break;

		strResultPath = szDosBuf;
		strResultPath += strPath.substr(2);

	} while (false);

	return strResultPath;
}

vector<string> FileSystem::getFilesInDirectory(const string& strFileDir, const char *szExt)
{
	if (strFileDir.empty())
		return std::vector<string>();

	std::vector<string> files;
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
	return 	std::vector<string>(files.begin(), files.end());
}

bool FileSystem::IsDirectoryExists(const std::string& strDir)
{
	if (strDir.empty())
		return false;
	DWORD dwAttrib = GetFileAttributesA(strDir.c_str());
	return (dwAttrib != INVALID_FILE_ATTRIBUTES) && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

bool FileSystem::IsFileExists(const string& strFilePath)
{
	if (strFilePath.empty())
		return false;
	DWORD dwAttrib = GetFileAttributesA(strFilePath.c_str());
	return dwAttrib != INVALID_FILE_ATTRIBUTES;
}

bool FileSystem::createDirectoryRecursive(std::string& strDir)
{
	if (!strDir.empty() && (strDir.back() == '\\' || strDir.back() == '/'))
		strDir.pop_back();

	if (CreateDirectoryA(strDir.c_str(), nullptr))
		return true;

	DWORD error = GetLastError();
	// 目录已存在，检查是否为文件夹
	if (error == ERROR_ALREADY_EXISTS)
	{
		DWORD attrib = GetFileAttributesA(strDir.c_str());
		return (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY);
	}
	// 路径不存在，创建父目录
	else if (error == ERROR_PATH_NOT_FOUND)
	{
		std::string strParentPath = getDirectory(strDir);
		if (strParentPath.empty())
			return false; // 父路径为空（如根目录）

		// 递归创建父目录并重试
		if (createDirectoryRecursive(strParentPath))
			return CreateDirectoryA(strDir.c_str(), nullptr);
	}
	return false;
}

string FileSystem::getDirectory(const string& strFilePath)
{
	if (strFilePath.empty())
		return string();
	size_t pos = strFilePath.find_last_of("/\\");
	if (pos != std::string::npos)
		return strFilePath.substr(0, pos + 1);
	return "";
}

string FileSystem::getFileName(const string& strFilePath)
{
	if (strFilePath.empty())
		return string();
	size_t pos = strFilePath.find_last_of("/\\");
	if (pos != std::string::npos)
		return strFilePath.substr(pos + 1);
	return "";
}

void FileSystem::createDirFromFilePath(const string &strFilePath)
{
	string strDirName = getDirectory(strFilePath);
	if (!IsDirectoryExists(strDirName))
		createDirectoryRecursive(strDirName);
}

HANDLE FileSystem::openOrCreateFile(const string& strFilePath, uint32_t uDesiredAccess, uint32_t uCreationDisposition,
	uint32_t uFlagsAndAttributes, uint32_t uShartMode)
{
	createDirFromFilePath(strFilePath);
	return CreateFileA(strFilePath.c_str(), uDesiredAccess, uShartMode, NULL,
		uCreationDisposition, uFlagsAndAttributes, NULL);
}

bool FileSystem::setFilePoint(HANDLE hFile, uint32_t uPoint, uint8_t uMoveMethod)
{
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	SetFilePointer(hFile, uPoint, 0, uMoveMethod);
	return true;
}

bool FileSystem::writeFile(HANDLE hFile, void* pData, uint32_t uSize)
{
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	return WriteFile(hFile, pData, uSize, nullptr, nullptr);
}

bool FileSystem::flushFile(HANDLE hFile)
{
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	return FlushFileBuffers(hFile);
}

bool FileSystem::closeFile(HANDLE hFile)
{
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	return CloseHandle(hFile);
}

bool FileSystem::readFile(HANDLE hFile, void* pData, uint32_t uSize)
{
	if (hFile == INVALID_HANDLE_VALUE || !pData || !uSize)
		return false;
	if (!ReadFile(hFile, pData, uSize, nullptr, nullptr))
		return false;
	return true;
}

bool FileSystem::delFile(const string& strFilePath)
{
	if (strFilePath.empty())
		return false;
	return ::DeleteFileA(strFilePath.c_str());
}

bool FileSystem::IsAbsolutePath(const std::string& strPath) 
{
#ifdef _WIN32
	// Windows逻辑：检查盘符路径或UNC路径
	if (strPath.size() >= 3 && strPath[1] == ':' && (strPath[2] == '/' || strPath[2] == '\\'))
		return true;
	if (strPath.size() >= 2 && (strPath[0] == '\\' && strPath[1] == '\\') || (strPath[0] == '/' && strPath[1] == '/'))
		return true;
	return false;
#else
	// Linux/Unix逻辑：检查是否以'/'开头
	return !strPath.empty() && strPath[0] == '/';
#endif
}

string FileSystem::relative2AbsolutePath(const std::string& strRelaPath)
{
	char szFullPath[MAX_PATH];
#ifdef _WIN32
	GetFullPathNameA(strRelaPath.c_str(), MAX_PATH, szFullPath, nullptr);
	return std::string(szFullPath);
#else
	realpath(strRelaPath.c_str(), full_path);
	return std::string(szFullPath);
#endif
}
