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
#include <stdio.h>
#include <stdint.h>
#include "ObjectMemPool.h"
#include "MemPoolCommon.h"
#include <unordered_map>
#include "PageMap.h"

class ThreadCache {
public:
	~ThreadCache();
	void* Allocate(size_t size);
	void* FetchFromCentralCache(size_t index, size_t size);

	void Deallocate(void* ptr);

	void ListTooLong(FreeList& list, size_t size);
private:
	FreeList m_freeLists[FREELISTS_NUM];

};
#ifdef _WIN32
#define THREAD_LOCAL static __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#include <pthread.h>
#define THREAD_LOCAL static __attribute__((thread))
#else
#define THREAD_LOCAL  static __thread
#endif
//thread local storage
THREAD_LOCAL ThreadCache m_TLSThreadCache;


class CentralCache {
public:

	Span* GetOneSpan(size_t index, size_t size);

	size_t FetchRangeObj(void*& begin, void*& end, size_t batch, size_t size);

	void ReleaseListToSpans(void* begin, size_t size);

	void* GetLargeMem(size_t size);
	 
	void ReleaseLargeMem(void* ptr);

	static CentralCache* GetInstance()
	{
		return &s_instCentralCache;
	}
private:
	SpanList m_spanLists[FREELISTS_NUM];

	static CentralCache s_instCentralCache;
private:
	CentralCache(){}
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;
};

class PageCache {
public:
	static PageCache* GetInstance()
	{
		return &s_InstPageCache;
	}

	Span* NewSpan(size_t kpages);

	Span* MapObjToSpan(void* obj);


	void ReleaseSpan(Span* span);

	std::mutex* Mutex()
	{
		return &m_mtx;
	}
private:
	SpanList m_pageLists[KPAGE];

	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;

	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	//std::map<PAGE_ID, Span*> _idSpanMap;
	// 
	// 

#if (_WIN32)

#if defined(_WIN64)

	TCMalloc_PageMap3<64 - PAGE_SHIFT> _idSpanMap;//
#else
	TCMalloc_PageMap2<32 - PAGE_SHIFT> _idSpanMap;//
#endif

#elif (__linux__)
// Linuxϵͳ�µĴ���
  // Ȼ���ж�λ��
#if (__x86_64__)
	TCMalloc_PageMap3<64 - PAGE_SHIFT> _idSpanMap;
#else
	TCMalloc_PageMap2<32 - PAGE_SHIFT> _idSpanMap;
#endif
#endif
	std::mutex m_mtx;

private:
	PageCache()	{}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;
	//ObjectMemPool<Span> _spanPool;

	static PageCache s_InstPageCache;
};

