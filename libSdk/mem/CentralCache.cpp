#ifdef _WIN32
#include <windows.h>
#endif
#include "ThreadCache.h"

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

void* ThreadCache::Allocate(size_t size)
{
	assert(size > 0);
	if (size <= MAX_BYTES)
	{
		size_t alignSize = SizeClass::RoundUp(size);
		size_t index = SizeClass::Index(size);
		if (!m_freeLists[index].Empty())
			return m_freeLists[index].PopFront();
		else
			return FetchFromCentralCache(index, alignSize);
	}
	return CentralCache::GetInstance()->GetLargeMem(size);
}

ThreadCache::~ThreadCache()
{
	for (uint8_t i = 0; i < FREELISTS_NUM ; i++)
	{
		if (!m_freeLists[i].Empty())
		{
			void* ptr = m_freeLists[i].PopFront();
			//PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
			Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
			size_t size = span->m_nObjSize;
			CentralCache::GetInstance()->ReleaseListToSpans(ptr, size);
			//ListTooLong(_freeLists[i], size);
		}
	}
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	size_t batch = min(m_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));

	if (batch == m_freeLists[index].MaxSize())
		m_freeLists[index].MaxSize()++;

	void* start = nullptr;
	void* end = nullptr;
	size_t actual_batch = CentralCache::GetInstance()->FetchRangeObj(start, end, batch, size);
	assert(actual_batch > 0);

	if (actual_batch > 1)
		m_freeLists[index].PushRangeFront(NextObj(start), end, actual_batch - 1);
	else
		assert(start == end);
	return start;
}

void ThreadCache::Deallocate(void* ptr)
{
	//PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
	if (!span)
		return;
	size_t size = span->m_nObjSize;
	assert(size > 0);

	if (size <= MAX_BYTES)
	{
		size_t index = SizeClass::Index(size);
		m_freeLists[index].PushFront(ptr);
		if (m_freeLists[index].MaxSize() <= m_freeLists[index].Size())
			ListTooLong(m_freeLists[index], size);
	}
	else
		CentralCache::GetInstance()->ReleaseLargeMem(ptr);
}

void ThreadCache::ListTooLong(FreeList& list, size_t size)
{
	void* begin;
	void* end;
	list.PopRangeFront(begin, end, list.MaxSize());
	CentralCache::GetInstance()->ReleaseListToSpans(begin, size);
}

CentralCache CentralCache::s_instCentralCache;

size_t CentralCache::FetchRangeObj(void*& begin, void*& end, size_t batch, size_t size)
{
	size_t index = SizeClass::Index(size);

	m_spanLists[index].Lock();
	Span* span = GetOneSpan(index, size);
	assert(span);
	assert(span->m_freeList != nullptr);

	void* cur = span->m_freeList;
	batch--;
	size_t actualNum = 1;
	begin = cur;
	while (batch > 0 && NextObj(cur) != nullptr)
	{
		cur = NextObj(cur);
		actualNum++;
		batch--;
	}
	span->m_freeList = NextObj(cur);
	NextObj(cur) = nullptr;
	end = cur;
	span->m_nUseCount += actualNum;

	m_spanLists[index].Unlock();
	return actualNum;
}


Span* CentralCache::GetOneSpan(size_t index, size_t size)
{
	Span* it = m_spanLists[index].Begin();
	while (it != m_spanLists[index].End())
	{
		if (it->m_freeList != nullptr)
		{
			return it;
		}
		it = it->m_next;
	}
	m_spanLists[index].Unlock();

	PageCache::GetInstance()->Mutex()->lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	PageCache::GetInstance()->Mutex()->unlock();

	char* start = (char*)(span->m_pageId << PAGE_SHIFT);
	size_t bytes = span->m_nPageNum << PAGE_SHIFT;
	char* end = start + bytes;

	span->m_freeList = start;
	start += size;
	void* tail = span->m_freeList;
	while (start <end)
	{
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}
	NextObj(tail) = nullptr;
	span->m_nObjSize = size;
	m_spanLists[index].Lock();
	m_spanLists[index].PushFront(span);
	return span;
}

void* CentralCache::GetLargeMem(size_t size)
{
	size_t alignSize = SizeClass::RoundUp(size);
	size_t kpage = alignSize >> PAGE_SHIFT;
	PageCache::GetInstance()->Mutex()->lock();
	Span* span = PageCache::GetInstance()->NewSpan(kpage);
	PageCache::GetInstance()->Mutex()->unlock();
	span->m_nObjSize = alignSize;
	return (void*)(span->m_pageId << PAGE_SHIFT);
}

void CentralCache::ReleaseLargeMem(void* ptr)
{
	//PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
	if (!span)
		return;
	size_t size = span->m_nObjSize;
	if (size > MAX_BYTES)
	{
		PageCache::GetInstance()->Mutex()->lock();
		PageCache::GetInstance()->ReleaseSpan(span);
		PageCache::GetInstance()->Mutex()->unlock();
	}
}

void CentralCache::ReleaseListToSpans(void* begin, size_t size)
{
	size_t index = SizeClass::Index(size);
	m_spanLists[index].Lock();
	while (begin != nullptr)
	{
		void* next = NextObj(begin);
		Span* span = PageCache::GetInstance()->MapObjToSpan(begin);
		if (!span)
			continue;
		NextObj(begin) = span->m_freeList;
		span->m_freeList = begin;
		span->m_nUseCount--;
		begin = next;
		if (span->m_nUseCount == 0)
		{
			m_spanLists[index].Erase(span);
			m_spanLists[index].Unlock();
			PageCache::GetInstance()->Mutex()->lock();
			PageCache::GetInstance()->ReleaseSpan(span);
			PageCache::GetInstance()->Mutex()->unlock();
			m_spanLists[index].Lock();

		}
	}
	m_spanLists[index].Unlock();
}


PageCache PageCache::s_InstPageCache;
Span* PageCache::NewSpan(size_t k)
{
	assert(k >= 1);
	if (k > KPAGE - 1)
	{
		void* ptr = SystemAlloc(k);
		Span* span = ::new Span;
		//Span* span = _spanPool.New();

		span->m_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->m_nPageNum = k;

	
		//_idSpanMap[span->m_pageId] = span;
		//_idSpanMap[span->m_pageId] = span;
		_idSpanMap.set(span->m_pageId, span);
		return span;
	}


	if (!m_pageLists[k].Empty())
	{
		Span* span = m_pageLists[k].PopFront();
	
		for (size_t i = 0; i < span->m_nPageNum; i++)
		{
			//_idSpanMap[span->m_pageId + i] = span;
			_idSpanMap.set(span->m_pageId + i, span);
		}
		span->m_isUse = true;
		return span;
	}

	for (size_t i = k + 1; i < KPAGE; i++)
	{
	
		if (!m_pageLists[i].Empty())
		{
			Span* span = m_pageLists[i].PopFront();
			Span* newSpan = ::new Span;
			//Span* newSpan = _spanPool.New();
			newSpan->m_pageId = span->m_pageId;
			newSpan->m_nPageNum = k;

			span->m_nPageNum -= k;
			span->m_pageId += k;
			m_pageLists[span->m_nPageNum].PushFront(span);

	
			//_idSpanMap[span->m_pageId] = span;
			//_idSpanMap[span->m_pageId + span->m_nPageNum - 1] = span;

			_idSpanMap.set(span->m_pageId, span);
			_idSpanMap.set(span->m_pageId + span->m_nPageNum - 1, span);
		
			for (size_t i = 0; i < newSpan->m_nPageNum; i++)
			{
				//_idSpanMap[newSpan->m_pageId + i] = newSpan;
				_idSpanMap.set(newSpan->m_pageId + i, newSpan);
			}

			return newSpan;
		}
	}


	void* ptr = SystemAlloc(KPAGE - 1);
	Span* span = ::new Span;
	//Span* span = _spanPool.New();
	span->m_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	span->m_nPageNum = KPAGE - 1;
	m_pageLists[span->m_nPageNum].PushFront(span);
	return NewSpan(k);

}


Span* PageCache::MapObjToSpan(void* obj)
{
	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;
	auto ret = (Span*)_idSpanMap.get(id);
	assert(ret != nullptr);
	return ret;
}


void PageCache::ReleaseSpan(Span* span)
{

	while (1)
	{
		PAGE_ID id = span->m_pageId;
		auto ret = (Span*)_idSpanMap.get(id);
		if (ret == nullptr)
			break;

		Span* prevSpan = ret;
		if (prevSpan->m_isUse == true)
			break;

		if (prevSpan->m_nPageNum + span->m_nPageNum > KPAGE - 1)
		{
			break;
		}
		
		span->m_pageId = prevSpan->m_pageId;
		span->m_nPageNum += prevSpan->m_nPageNum;

		m_pageLists[prevSpan->m_nPageNum].Erase(prevSpan);
		//delete prevSpan;
		//SystemFree((void*)(prevSpan->m_pageId << PAGE_SHIFT));
		delete prevSpan;
		
	
		//_spanPool.Delete(prevSpan);
	}

	while (1)
	{

		PAGE_ID nextId = span->m_pageId + span->m_nPageNum;

		auto ret = (Span*)_idSpanMap.get(nextId);
		if (ret == nullptr)
		{
			break;
		}

		//Span* nextSpan = ret->second;
		Span* nextSpan = ret;
		if (nextSpan->m_isUse == true)
			break;

		if (nextSpan->m_nPageNum + span->m_nPageNum > KPAGE - 1)
			break;

		span->m_nPageNum += nextSpan->m_nPageNum;
		m_pageLists[nextSpan->m_nPageNum].Erase(nextSpan);
		delete nextSpan;
		//_spanPool.Delete(nextSpan);


	}
	m_pageLists[span->m_nPageNum].PushFront(span);
	span->m_isUse = false;

	/*_idSpanMap[span->m_pageId] = span;
	_idSpanMap[span->m_pageId + span->m_nPageNum - 1] = span;*/

	_idSpanMap.set(span->m_pageId, span);
	_idSpanMap.set(span->m_pageId + span->m_nPageNum - 1, span);

}

