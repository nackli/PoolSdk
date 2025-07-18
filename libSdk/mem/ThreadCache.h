#pragma once
#include <stdio.h>
#include <stdint.h>
#include "ObjectMemPool.h"
#include "MemPoolCommon.h"
#include <unordered_map>
#include "PageMap.h"

//Spanһ����ȵĴ���ڴ�
// ������ҳΪ��λ�Ĵ���ڴ�
// �����������ҳ����ڴ��Ƚṹ
class ThreadCache {
public:
	~ThreadCache();
	//��thread cache�����ڴ�
	void* Allocate(size_t size);

	/*
	* ��thread cacheû�ж�ӦС���ڴ�ʱ�����central cache�л�ȡС���ڴ�
	index��	����ȷ�������Ļ�����ĸ�Ͱ�н��л�ȡ�ڴ��Ĵ�С��ȷ�����ڴ��Ĵ�С
	size��	��ȡָ���ڴ��С�ĸ�����
	*/
	void* FetchFromCentralCache(size_t index, size_t size);

	//	------------------------------------------------------------------------ -
		//�ͷ��߼�
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
//thread local storage����(TLS) ��Ȼ������ȫ�־�̬�����������ܹ���֤ÿ������ֻ��һ�� 
THREAD_LOCAL ThreadCache m_TLSThreadCache;


class CentralCache {
public:
	//�ڶ�Ӧ��Ͱ�л�ȡһ������С�����Span
	Span* GetOneSpan(size_t index, size_t size);
	/*
	* ��ȡһ��С����
	begin��		��Ż�ȡ��һ���ڴ��ĵ�ַ
	end��		��Ż�ȡ���һ���ڴ��ĵ�ַ
	batch��	��Ҫ�����Ļ����н��л�ȡ�ڴ�������
	size��		ÿ���ڴ��Ĵ�С
	ret��		ʵ�ʴ�central���л�ȡ�ڴ��ĸ���
	*/
	size_t FetchRangeObj(void*& begin, void*& end, size_t batch, size_t size);

	//�ͷ��߼�
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
	//��ȡһ���µ�Span
	Span* NewSpan(size_t kpages);

	//���ݵ�ַ������Span
	Span* MapObjToSpan(void* obj);

	//�ͷ�Span
	void ReleaseSpan(Span* span);

	std::mutex* Mutex()
	{
		return &m_mtx;
	}
private:
	SpanList m_pageLists[KPAGE];

	//pageId�����õ�Span
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;

	//-------------���������������Ż�---------------------//
	//std::unordered_map<PAGE_ID, Span*> _idSpanMap;
	//std::map<PAGE_ID, Span*> _idSpanMap;
	// 
	// 
//���������Ż�
// �����жϲ���ϵͳ
#if (_WIN32)
// Windowsϵͳ�µĴ���
  // Ȼ���ж�λ��
#if defined(_WIN64)
/*
*   32λϵͳ�£�һ��ֻ��2^19��ҳ����64λϵͳ�¿���������2^51��ҳ����ζ�Ż���������Ҫ�õ�2^51��void*ָ�룬
	ÿ��ָ��ռ��8�ֽڣ���Ҫ2^54�ֽ��ڴ���ܴ�������֮���ָ�롣��Ȼ��64λϵͳ�µĻ�����Ҫ�������⴦��
*/
	TCMalloc_PageMap3<64 - PAGE_SHIFT> _idSpanMap;//64λϵͳ�µ���3�������
#else
	TCMalloc_PageMap2<32 - PAGE_SHIFT> _idSpanMap;//32λϵͳ�µ���2�������
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
	//ʹ�ö����ڴ������New
	//ObjectMemPool<Span> _spanPool;

	static PageCache s_InstPageCache;
};

