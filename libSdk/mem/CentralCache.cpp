#ifdef _WIN32
#include <windows.h>
#endif
#include "ThreadCache.h"

void* ThreadCache::Allocate(size_t size)
{
	//申请的内存必须合法
	assert(size > 0);
	if (size <= MAX_BYTES)
	{
		size_t alignSize = SizeClass::RoundUp(size);
		size_t index = SizeClass::Index(size);
		//如果桶上有小块内存则返回给用户
		if (!m_freeLists[index].Empty())
			return m_freeLists[index].PopFront();
		else//链表中没有空闲内存需要向CentralCache申请
			return FetchFromCentralCache(index, alignSize);
	}
	return CentralCache::GetInstance()->GetLargeMem(size);
}

ThreadCache::~ThreadCache()
{
	for (int i = 0; i < FREELISTS_NUM ; i++)//将线程资源层所有桶里的内存块都归还给中心资源层
	{
		if (!m_freeLists[i].Empty())
		{
			void* ptr = m_freeLists[i].PopFront();
			PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
			Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
			size_t size = span->m_nObjSize;
			CentralCache::GetInstance()->ReleaseListToSpans(ptr, size);
			//ListTooLong(_freeLists[i], size);
		}
	}
}
//从CentralCache中获取多个小块内存
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	/* 慢启动
	* 慢开始反馈调节算法
	1、最开始不会一次像central cache一次进行批量要太多，因为要太多可能用不完导致浪费
	2、如果进行持续请求这种内存的大小，batchNum就会增长，直到上限
	3、size越大，一次向central cache要的batchNum 就越小
	4、size越小，一次向central cache要的batchNum 就越大

	* 每个桶有自己的maxSize，表示申请过多少次，每次成功申请都会增加其值
	* 要申请的空闲内存块数 = min(maxSize,MAX_BYTES/size)
	* MAX_BYTES/size:小对象申请的上限大，大对象申请的上限小
	*/
	size_t batch = min(m_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));

	if (batch == m_freeLists[index].MaxSize())
		m_freeLists[index].MaxSize()++;

	void* start = nullptr;
	void* end = nullptr;
	//FetchRangeObj是CentralCache的方法
	size_t actual_batch = CentralCache::GetInstance()->FetchRangeObj(start, end, batch, size);
	//如果actual_num为0，则表示内存获取失败，
	// 1.要么申请内存没申请到，则在申请处会抛异常的，不会走到这
	// 2.要么程序逻辑错误，应该断言检查
	assert(actual_batch > 0);

	if (actual_batch > 1)
		m_freeLists[index].PushRangeFront(NextObj(start), end, actual_batch - 1);
	else
		assert(start == end);
	return start;
}

//释放对象内存给ThreadCache
void ThreadCache::Deallocate(void* ptr)
{
	PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
	size_t size = span->m_nObjSize;
	assert(size > 0);

	if (size <= MAX_BYTES)
	{
		size_t index = SizeClass::Index(size);
		m_freeLists[index].PushFront(ptr);

		//如果现在ThreadCache中自由链表空闲内存过多就将其打包释放给CentralCache
		//此实现细节只考虑了自由链表过长的情况，还可以加上自由链表所含闲置内存达到一个阈值时触发
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
//获取一些小块内存
size_t CentralCache::FetchRangeObj(void*& begin, void*& end, size_t batch, size_t size)
{
	size_t index = SizeClass::Index(size);

	m_spanLists[index].Lock();
	Span* span = GetOneSpan(index, size);
	assert(span);
	assert(span->m_freeList != nullptr);

	//从span的小块内存中，选取一部分
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

//从spanList上获取一个含有空闲小块内存的span
Span* CentralCache::GetOneSpan(size_t index, size_t size)
{
	//首先找对应的SpanList，查看是否有Span含有小对象内存
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
	//走到这里说明没有空闲Span了，需要向PageCache要

	PageCache::GetInstance()->Mutex()->lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	PageCache::GetInstance()->Mutex()->unlock();

	//从PageCache获取到Span后，将Span所管理的大块内存，切成许多小块内存，挂在freeList上

	//获取从PageCache得到的大块内存的首尾地址
	char* start = (char*)(span->m_pageId << PAGE_SHIFT);
	size_t bytes = span->m_nPageNum << PAGE_SHIFT;
	char* end = start + bytes;

	//将大块内存切分为一块一块小内存，并链接到freeList上
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
	PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
	size_t size = span->m_nObjSize;
	if (size > MAX_BYTES)
	{
		PageCache::GetInstance()->Mutex()->lock();
		PageCache::GetInstance()->ReleaseSpan(span);
		PageCache::GetInstance()->Mutex()->unlock();
	}
}

//将ThreadCache释放的小块内存重新挂到Span上，当此span小块内存都被归还时，再释放到PageCache中
void CentralCache::ReleaseListToSpans(void* begin, size_t size)
{
	size_t index = SizeClass::Index(size);
	m_spanLists[index].Lock();
	//将每块小内存通过映射找到其对应的span，并挂上去
	while (begin != nullptr)
	{
		void* next = NextObj(begin);
		Span* span = PageCache::GetInstance()->MapObjToSpan(begin);
		//头插入span的freeList上
		NextObj(begin) = span->m_freeList;
		span->m_freeList = begin;
		span->m_nUseCount--;
		begin = next;
		//如果span的所有小块内存均为被使用则将其回收到PageCache中
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

		//-------------基数树进行性能优化---------------------//
		//_idSpanMap[span->m_pageId] = span;
		//_idSpanMap[span->m_pageId] = span;
		_idSpanMap.set(span->m_pageId, span);
		return span;
	}

	//检查第一个桶有没有Span
	if (!m_pageLists[k].Empty())
	{
		Span* span = m_pageLists[k].PopFront();
		//将分出给CentralCache的span每个页与其指针映射起来
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
		//如果比kpages大的桶中有span则直接分割span
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

			//如果是在PageCache中保存的页，只需要将其首尾页进行映射即可
			//_idSpanMap[span->m_pageId] = span;
			//_idSpanMap[span->m_pageId + span->m_nPageNum - 1] = span;

			_idSpanMap.set(span->m_pageId, span);
			_idSpanMap.set(span->m_pageId + span->m_nPageNum - 1, span);
			//将分出给CentralCache的span每个页与其指针映射起来
			for (size_t i = 0; i < newSpan->m_nPageNum; i++)
			{
				//_idSpanMap[newSpan->m_pageId + i] = newSpan;
				_idSpanMap.set(newSpan->m_pageId + i, newSpan);
			}

			return newSpan;
		}
	}
	//如果比kpages大的桶中没有span，则向系统申请

	//像系统申请的内存根据其地址，直接映射为页号
	//只要保证PAGE和系统页面是一样大小就行

	void* ptr = SystemAlloc(KPAGE - 1);
	Span* span = ::new Span;
	//Span* span = _spanPool.New();
	span->m_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	span->m_nPageNum = KPAGE - 1;
	m_pageLists[span->m_nPageNum].PushFront(span);
	return NewSpan(k);

}

//将对象地址转换为页号，寻找对应的span
Span* PageCache::MapObjToSpan(void* obj)
{
	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;
	auto ret = (Span*)_idSpanMap.get(id);
	assert(ret != nullptr);
	return ret;
}

//将CentralCache传来的空闲span与页号相邻的span合并
void PageCache::ReleaseSpan(Span* span)
{
	//先将小于此span第一页的相邻页合并
	while (1)
	{
		PAGE_ID id = span->m_pageId;

		//进行优化
		auto ret = (Span*)_idSpanMap.get(id);
		if (ret == nullptr)
			break;

		Span* prevSpan = ret;
		if (prevSpan->m_isUse == true)
			break;

		// 合并出超过128页的span没办法管理，不合并了
		if (prevSpan->m_nPageNum + span->m_nPageNum > KPAGE - 1)
		{
			break;
		}
		

		//合并逻辑
		span->m_pageId = prevSpan->m_pageId;
		span->m_nPageNum += prevSpan->m_nPageNum;

		m_pageLists[prevSpan->m_nPageNum].Erase(prevSpan);
		//delete prevSpan;
		//SystemFree((void*)(prevSpan->m_pageId << PAGE_SHIFT));
		delete prevSpan;
		
	
		//_spanPool.Delete(prevSpan);
	}

	//将在此span页之后的相邻span合并 向后合并
	while (1)
	{

		PAGE_ID nextId = span->m_pageId + span->m_nPageNum;

		//-------------基数树进行性能优化---------------------//
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
	//合并后要修改pageId和span的索引，不然会找到之前被释放的span
	m_pageLists[span->m_nPageNum].PushFront(span);
	span->m_isUse = false;

	//-------------基数树进行性能优化---------------------//
	/*_idSpanMap[span->m_pageId] = span;
	_idSpanMap[span->m_pageId + span->m_nPageNum - 1] = span;*/

	_idSpanMap.set(span->m_pageId, span);
	_idSpanMap.set(span->m_pageId + span->m_nPageNum - 1, span);

}

