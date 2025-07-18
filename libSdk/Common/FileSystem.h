#pragma once
#ifndef __PLATFORM_FILE_SYSTEM_H_
#define __PLATFORM_FILE_SYSTEM_H_
#include <string>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#endif
using namespace std;

#ifdef _WIN32		
	string DosPathToNtPath(const string& strPath);
	string NtPathToDosPath(const string& strPath);
#endif	
	std::string getDirFromFilePath(const std::string& filepath);
	vector<string> getFilesInDirectory(const string& strDir, const char* szExt);
	bool IsDirectoryExists(const string& strDir);
	bool IsFileExists(const string& strDir);
	bool createDirectoryRecursive(string& strDir);
	string getDirectory(const string& strFilePath);
	string getFileName(const string& strFilePath);
	void createDirFromFilePath(const string &strFilePath);
#ifdef _WIN32	
	HANDLE openOrCreateFile(const string &strFilePath, uint32_t uDesiredAccess, uint32_t uCreationDisposition,
		uint32_t uFlagsAndAttributes, uint32_t uShartMode = 0);
	bool setFilePoint(HANDLE hFile, uint32_t uPoint, uint8_t uMoveMethod);
	bool writeFile(HANDLE hFile, void* pData, uint32_t uSize);
	bool flushFile(HANDLE hFile);
	bool closeFile(HANDLE);
	bool readFile(HANDLE, void* pData, uint32_t uSize);
	bool delFile(const string& strFilePath);
#endif

	bool IsAbsolutePath(const std::string& strPath);
	string relative2AbsolutePath(const std::string& strRelaPath);	
#endif

