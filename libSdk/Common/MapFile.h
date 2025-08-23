#pragma once 
#ifndef __MAPPING_FILE_H_
#define __MAPPING_FILE_H_
#include <stdint.h>
#include <mutex>
#include <atomic>
#include <mutex>
#include <utility>
#include <condition_variable>
typedef void* MAPHANDLE;
class MapFile
{
public:
	MapFile();
	~MapFile();
	bool openOrCreateMap(const char* szMapName, int iMaxSize,  const char* szFileName = nullptr);
	bool writeMap(const char *szData);
	std::pair<char *,uint32_t> readMap(bool &bDeleteMem);//bool is true,must delete []
	bool moveReadOffset(uint32_t uOffset);
	uint64_t getCurFreeMem();
	uint32_t getCurReadOffset();
	void setCurReadOffset(uint64_t);
	bool closeMap();
private:
	MAPHANDLE m_hFileMap;
	std::atomic_ullong m_uWriteOffset;
	uint64_t m_uReadOffset;
	std::mutex m_lockMap;
	uint32_t m_uMemMaxSize;
};
#endif