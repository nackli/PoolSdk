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
#pragma once
#ifndef __PLATFORM_FILE_SYSTEM_H_
#define __PLATFORM_FILE_SYSTEM_H_
#include <string>
#include <list>
#include <map>
#ifdef _WIN32
#include <Windows.h>
#endif
#include <stdint.h>
namespace FileSystem
{

typedef struct FILEINFO
{
	std::string strFileName;
#ifdef _WIN32
	FILETIME lastWriteTime;
#else
	uint64_t lastWriteTime;
#endif
}FILEINFO,*LP_FILEINFO;
using FileInfoList = std::list<FILEINFO>;
using MAPSTRING = std::map<std::string, std::string>;
#ifdef _WIN32		
	string DosPathToNtPath(const string& strPath);
	string NtPathToDosPath(const string& strPath);
#endif	
	std::string getDirFromFilePath(const std::string& filepath);
	FileInfoList getFilesInCurDir(const std::string& strFilePathAndReg, const std::string& strExt,bool bOnlyFileName = false);
	bool IsDirectoryExists(const std::string& strDir);
	bool IsFileExists(const std::string& strDir);
	bool createDirectoryRecursive(std::string& strDir);
	std::string getDirectory(const std::string& strFilePath);
	std::string getFileName(const std::string& strFilePath);
	void createDirFromFilePath(const std::string &strFilePath);
	MAPSTRING parseConfig(const std::string& path); 
#ifdef _WIN32	
	HANDLE openOrCreateFile(const std::string &strFilePath, uint32_t uDesiredAccess, uint32_t uCreationDisposition,
		uint32_t uFlagsAndAttributes, uint32_t uShartMode = 0);
	bool setFilePoint(HANDLE hFile, uint32_t uPoint, uint8_t uMoveMethod);
	bool writeFile(HANDLE hFile, void* pData, uint32_t uSize);
	bool readFile(HANDLE, void* pData, uint32_t uSize);
	bool flushFile(HANDLE hFile);
	size_t getFileSize(HANDLE hFile);
	bool closeFile(HANDLE);
	bool winDelFile(const std::string& strFilePath);
	bool winMoveFile(const std::string& strOldFilePath, const string& strNewFilePath);
#endif
	FILE* openOrCreateFile(const std::string& strFilePath, const char *szFlags = "ab+");
	FILE* openOrCreateFile(const char * szFilePath, const char* szFlags = "ab+");
	FILE* reopenOrCreateFile(const char* szFilePath, FILE* hFile, const char* szFlags = "ab+");
	bool writeFile(FILE* hFile, void* pData, uint32_t uSize);
	bool readFile(FILE* hFile, void* pData, uint32_t uSize);
	bool flushFile(FILE* hFile, bool bForcedDisk = false);
	bool fseekFile(FILE* hFile,int iPos,int origin);
	long getFileCurPos(FILE* hFile);
	void rewindFile(FILE* hFile);
	size_t getFileSize(FILE* hFile);
	bool fgetsFile(FILE* hFile, char* szData, size_t uSize);

	bool IsEofFile(FILE* hFile);
	bool closeFile(FILE *);
	bool moveFile(const char* szOldFile, const char* szNewFile);
	bool delFile(const char* szFilePath);
	bool IsAbsolutePath(const std::string& strPath);
	std::string relative2AbsolutePath(const std::string& strRelaPath);	
#endif
}
