#pragma once
#include <stdio.h>
#include <stdint.h>
#include "ObjectMemPool.h"
#include "MemPoolCommon.h"
#include <unordered_map>
#include "PageMap.h"

//Span一个跨度的大块内存
// 管理以页为单位的大块内存
// 管理多个连续页大块内存跨度结构
class ThreadCache {
public:
	~ThreadCache();
	//向thread cache申请内存
	void* Allocate(size_t size);

	/*
	* 当thread cache没有对应小块内存时，会从central cache中获取小块内存
	index：	用于确定从中心缓存的哪个桶中进行获取内存块的大小，确定了内存块的大小
	size：	获取指定内存大小的个数。
	*/
	void* FetchFromCentralCache(size_t index, size_t size);

	//	------------------------------------------------------------------------ -
		//释放逻辑
	void Deallocate(void* ptr);

	void ListTooLong(FreeList& list, size_t size);
private:
	FreeList m_freeLists[FREELISTS_NUM];

};
#ifdef _WIN32
#define THREAD_LOCAL static __declspec(thread)
#elif defined(__GNUC__) || defined(__clang__)
#define THREAD_LOCAL static __thread
#else
#define THREAD_LOCAL 
#endif
//thread local storage技术(TLS) 虽然声明是全局静态变量，但是能够保证每个进程只有一份 
THREAD_LOCAL ThreadCache m_TLSThreadCache;


class CentralCache {
public:
	//在对应的桶中获取一个含有小对象的Span
	Span* GetOneSpan(size_t index, size_t size);
	/*
	* 获取一批小对象
	begin：		存放获取第一个内存块的地址
	end：		存放获取最后一个内存块的地址
	batch：	想要从中心缓存中进行获取内存块的数量
	size：		每个内存块的大小
	ret：		实际从central进行获取内存块的个数
	*/
	size_t FetchRangeObj(void*& begin, void*& end, size_t batch, size_t size);

	//释放逻辑
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
	//获取一个新的Span
	Span* NewSpan(size_t kpages);

	//根据地址索引到Span
	Span* MapObjToSpan(void* obj);

	//释放Span
	void ReleaseSpan(Span* span);

	std::mutex* Mutex()
	{
		return &m_mtx;
	}
private:
	SpanList m_pageLists[KPAGE];

	//pageId索引得到Span
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;

	//-------------基数树进行性能优化---------------------//
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	//std::map<PAGE_ID, Span*> _idSpanMap;
	// 
	// 
//进行性能优化
// 首先判断操作系统
#if (_WIN32)
// Windows系统下的代码
  // 然后判断位数
#if defined(_WIN64)
/*
*   32位系统下，一共只有2^19个页。但64位系统下可以有足足2^51个页，意味着基数树内需要用到2^51个void*指针，
	每个指针占用8字节，需要2^54字节内存才能存放下如此之多的指针。显然，64位系统下的基数树要进行特殊处理
*/
	TCMalloc_PageMap3<64 - PAGE_SHIFT> _idSpanMap;//64位系统下调用3层基数树
#else
	TCMalloc_PageMap2<32 - PAGE_SHIFT> _idSpanMap;//32位系统下调用2层基数树
#endif

#elif (__linux__)
// Linux系统下的代码
  // 然后判断位数
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
	//使用定长内存池脱离New
	//ObjectMemPool<Span> _spanPool;

	static PageCache s_InstPageCache;
};

