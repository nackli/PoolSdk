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
#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#endif
#include "FileSystem.h"
#include "MapFile.h"
#include "FileLogger/FileLogger.h"

/*********************************************************************************************************************/
#define MAX_READ_MEM_BYTE					0x300000
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE				-1
#endif
/*********************************************************************************************************************/
typedef struct
{
#ifdef _WIN32
	HANDLE hFile;
	HANDLE hMap;
#else
	int hFile;	
#endif
	void* pMem;
}MAP_HANDLE, *LP_MAP_HANDLE;
/*********************************************************************************************************************/
MapFile::MapFile():m_hFileMap(nullptr),m_uWriteOffset(0), m_uReadOffset(0), m_uMemMaxSize(0)
{
}

MapFile::~MapFile()
{
	closeMap();
}

bool MapFile::openOrCreateMap(const char* szMapName, int iMaxSize, const char* szFileName)
{
	if (!szMapName)
		return false;

	LP_MAP_HANDLE lpMap = new MAP_HANDLE;
	if (!lpMap)
		return false;
	m_uMemMaxSize = iMaxSize - 1;	
	m_hFileMap = lpMap;	
#ifdef _WIN32
	
	if (szFileName)
		lpMap->hFile = FileSystem::openOrCreateFile(szFileName, GENERIC_WRITE | GENERIC_READ, OPEN_ALWAYS, 0, 0);
	else
		lpMap->hFile = INVALID_HANDLE_VALUE;

	if(iMaxSize)
		lpMap->hMap = CreateFileMappingA(lpMap->hFile, NULL, PAGE_READWRITE, 0, iMaxSize, szMapName);
	else
		lpMap->hMap = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, false, szMapName);

	if (!lpMap->hMap || lpMap->hMap == INVALID_HANDLE_VALUE)
		goto end;

	lpMap->pMem = (char*)MapViewOfFile(lpMap->hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, (DWORD)0, 0);
	if (!lpMap->pMem)
		goto end;
#else
    if(!szFileName)
		szFileName = "/dev/xg_map";
    lpMap->hFile= open(szFileName, O_RDWR | O_CREAT, 0666);
	if(lpMap->hFile < 0)
	{
		perror("open dev fail\n"); 
		return false;
	}

	if(ftruncate(lpMap->hFile, m_uMemMaxSize) < 0)
	{
		perror("ftruncate fail\n"); 
		printf("m_uMemMaxSize = %d",m_uMemMaxSize);
		goto end;
	}
	
	
	lpMap->pMem =  (char *)mmap(NULL, m_uMemMaxSize, PROT_READ | PROT_WRITE, MAP_SHARED, lpMap->hFile, 0);
    if (lpMap->pMem  == MAP_FAILED) 
	{
		perror("open mmap fail\n"); 
		close(lpMap->hFile);
		goto end;
	}
#endif
	return true;
end:
	closeMap();
	LOG_ERROR_S("Init write fail");
	return false;
}

bool MapFile::writeMap(const char* szData)
{
	LP_MAP_HANDLE lpMap = (LP_MAP_HANDLE)m_hFileMap;
	if (!lpMap || !lpMap->pMem || !szData || !*szData)
	{
		LOG_ERROR_S("Write map data error");
		return false;
	}

	int iWriteLen = strlen(szData);
	int iCurMemPos = m_uWriteOffset.load() % m_uMemMaxSize;
	int iMemFree = m_uMemMaxSize - iCurMemPos + 1;
	char* pMemAddr = nullptr;

	if (iMemFree > iWriteLen)
	{
		pMemAddr = (char*)lpMap->pMem + iCurMemPos;
		std::lock_guard<std::mutex> lock(m_lockMap);
		memcpy(pMemAddr, szData, iWriteLen);
	}
	else
	{
		std::lock_guard<std::mutex> lock(m_lockMap);
		pMemAddr = (char*)lpMap->pMem + iCurMemPos;
		memcpy(pMemAddr, szData, iMemFree);

		int iFreeWrite = iWriteLen - iMemFree + 1;
		memcpy(lpMap->pMem, szData + iMemFree, iFreeWrite);
	}
	m_uWriteOffset.fetch_add(iWriteLen);
	return true;
}

std::pair<char*, uint32_t> MapFile::readMap(bool& bDeleteMem)
{
	LP_MAP_HANDLE lpMap = (LP_MAP_HANDLE)m_hFileMap;
	if (!lpMap || !lpMap->pMem )
	{
		LOG_ERROR_S("Read map param error");
		return pair<char*, uint32_t>();
	}
	uint32_t uActMemSize = MAX_READ_MEM_BYTE;
	uint64_t uMemFree = getCurFreeMem();
	bDeleteMem = false;
	if (uMemFree < uActMemSize)
		uActMemSize = (uint32_t)uMemFree;

	
	
	if (m_uReadOffset < m_uWriteOffset.load())
	{
		if (uActMemSize + m_uReadOffset > m_uWriteOffset.load())
			uActMemSize = (size_t)(m_uWriteOffset.load() - m_uReadOffset);

		char* pMemData = nullptr;
		uint32_t iMemPos = m_uReadOffset % m_uMemMaxSize;
		if (iMemPos + uActMemSize < m_uMemMaxSize)
		{
			pMemData = (char*)lpMap->pMem + iMemPos;
			return pair<char*, uint32_t>(pMemData, uActMemSize);
		}
		else
		{
			std::lock_guard<std::mutex> lock(m_lockMap);
			char* pDataRet = new char[uActMemSize + 1];

			pMemData = (char*)lpMap->pMem + iMemPos;
			uint32_t uFreeMem = m_uMemMaxSize - iMemPos + 1;
			memcpy(pDataRet, pMemData, uFreeMem);
			memcpy(pDataRet + uFreeMem, lpMap->pMem, uActMemSize - uFreeMem);
			*(pDataRet + uActMemSize) = '\0';
			bDeleteMem = true;
			return pair<char*, uint32_t>(pDataRet, uActMemSize);
		}
	}
	return pair<char*, uint32_t>();
}

bool MapFile::closeMap()
{
	if (!m_hFileMap)
		return false;
	LOG_ERROR_S("Stop map");
	LP_MAP_HANDLE lpMap = (LP_MAP_HANDLE)m_hFileMap;
#ifdef _WIN32
	if (lpMap->pMem)
	{
		UnmapViewOfFile(lpMap->pMem);
		lpMap->pMem = nullptr;
	}

	if (lpMap->hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(lpMap->hFile);
		lpMap->hFile = INVALID_HANDLE_VALUE;
	}

	if (lpMap->hMap != INVALID_HANDLE_VALUE)
	{
		CloseHandle(lpMap->hMap);
		lpMap->hMap = INVALID_HANDLE_VALUE;
	}
#else
	if (lpMap->hFile != INVALID_HANDLE_VALUE)
	{
		close(lpMap->hFile);
		lpMap->hFile = INVALID_HANDLE_VALUE;
	}

	if (lpMap->pMem)
	{
		munmap(lpMap->pMem,m_uMemMaxSize);
		lpMap->pMem = nullptr;
	}
#endif
	if (m_hFileMap)
	{
		delete (LP_MAP_HANDLE)m_hFileMap;
		m_hFileMap = nullptr;
	}
	return true;
}

bool MapFile::moveReadOffset(uint32_t uOffset)
{
	if (m_uReadOffset + uOffset <= m_uWriteOffset.load())
		m_uReadOffset += uOffset;
	else
		m_uReadOffset = m_uWriteOffset.load();
	return true;
}

uint64_t MapFile::getCurFreeMem()
{
	if(m_uWriteOffset.load() > m_uReadOffset)
		return (m_uWriteOffset.load() - m_uReadOffset);
	return 0;
}

uint32_t MapFile::getCurReadOffset()
{
	return m_uReadOffset % m_uMemMaxSize;
}

void MapFile::setCurReadOffset(uint64_t uReadOffset)
{
	if (uReadOffset > m_uReadOffset && uReadOffset < m_uWriteOffset.load())
		m_uReadOffset = uReadOffset;
}