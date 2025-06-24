#ifdef _WIN32
#include <windows.h>
#endif
#include "ThreadCache.h"

void* ThreadCache::Allocate(size_t size)
{
	//������ڴ����Ϸ�
	assert(size > 0);
	if (size <= MAX_BYTES)
	{
		size_t alignSize = SizeClass::RoundUp(size);
		size_t index = SizeClass::Index(size);
		//���Ͱ����С���ڴ��򷵻ظ��û�
		if (!_freeLists[index].Empty())
		{
			return _freeLists[index].PopFront();
		}
		//������û�п����ڴ���Ҫ��CentralCache����
		else
		{
			return FetchFromCentralCache(index, alignSize);
		}
	}
	return CentralCache::GetInstance()->GetLargeMem(size);
}

//��CentralCache�л�ȡ���С���ڴ�
void* ThreadCache::FetchFromCentralCache(size_t index, size_t size)
{
	/* ������
	* ����ʼ���������㷨
	1���ʼ����һ����central cacheһ�ν�������Ҫ̫�࣬��ΪҪ̫������ò��굼���˷�
	2��������г������������ڴ�Ĵ�С��batchNum�ͻ�������ֱ������
	3��sizeԽ��һ����central cacheҪ��batchNum ��ԽС
	4��sizeԽС��һ����central cacheҪ��batchNum ��Խ��

	* ÿ��Ͱ���Լ���maxSize����ʾ��������ٴΣ�ÿ�γɹ����붼��������ֵ
	* Ҫ����Ŀ����ڴ���� = min(maxSize,MAX_BYTES/size)
	* MAX_BYTES/size:С������������޴󣬴�������������С
	*/
	size_t batch = min(_freeLists[index].MaxSize(), SizeClass::NumMoveSize(size));

	if (batch == _freeLists[index].MaxSize()) 
		_freeLists[index].MaxSize()++;

	void* start = nullptr;
	void* end = nullptr;
	//FetchRangeObj��CentralCache�ķ���
	size_t actual_batch = CentralCache::GetInstance()->FetchRangeObj(start, end, batch, size);
	//���actual_numΪ0�����ʾ�ڴ��ȡʧ�ܣ�
	// 1.Ҫô�����ڴ�û���뵽���������봦�����쳣�ģ������ߵ���
	// 2.Ҫô�����߼�����Ӧ�ö��Լ��
	assert(actual_batch > 0);

	if (actual_batch > 1)
		_freeLists[index].PushRangeFront(NextObj(start), end, actual_batch - 1);
	else
		assert(start == end);
	return start;
}

//�ͷŶ����ڴ��ThreadCache
void ThreadCache::Deallocate(void* ptr)
{
	PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
	size_t size = span->_objSize;
	assert(size > 0);

	if (size <= MAX_BYTES)
	{
		size_t index = SizeClass::Index(size);
		_freeLists[index].PushFront(ptr);

		//�������ThreadCache��������������ڴ����ͽ������ͷŸ�CentralCache
		//��ʵ��ϸ��ֻ�����������������������������Լ��������������������ڴ�ﵽһ����ֵʱ����
		if (_freeLists[index].MaxSize() <= _freeLists[index].Size())
		{
			ListTooLong(_freeLists[index], size);
		}
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

CentralCache CentralCache::_sInst;
//��ȡһЩС���ڴ�
size_t CentralCache::FetchRangeObj(void*& begin, void*& end, size_t batch, size_t size)
{
	size_t index = SizeClass::Index(size);

	_spanLists[index].Mutex().lock();
	Span* span = GetOneSpan(index, size);
	assert(span);
	assert(span->_freeList != nullptr);

	//��span��С���ڴ��У�ѡȡһ����
	void* cur = span->_freeList;
	batch--;
	size_t actualNum = 1;
	begin = cur;
	while (batch > 0 && NextObj(cur) != nullptr)
	{
		cur = NextObj(cur);
		actualNum++;
		batch--;
	}
	span->_freeList = NextObj(cur);
	NextObj(cur) = nullptr;
	end = cur;
	span->_useCount += actualNum;

	_spanLists[index].Mutex().unlock();
	return actualNum;
}

//��spanList�ϻ�ȡһ�����п���С���ڴ��span
Span* CentralCache::GetOneSpan(size_t index, size_t size)
{
	//�����Ҷ�Ӧ��SpanList���鿴�Ƿ���Span����С�����ڴ�
	Span* it = _spanLists[index].Begin();
	while (it != _spanLists[index].End())
	{
		if (it->_freeList != nullptr)
		{
			return it;
		}
		it = it->_next;
	}
	_spanLists[index].Mutex().unlock();
	//�ߵ�����˵��û�п���Span�ˣ���Ҫ��PageCacheҪ

	PageCache::GetInstance()->Mutex()->lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size));
	PageCache::GetInstance()->Mutex()->unlock();

	//��PageCache��ȡ��Span�󣬽�Span������Ĵ���ڴ棬�г����С���ڴ棬����freeList��

	//��ȡ��PageCache�õ��Ĵ���ڴ����β��ַ
	char* start = (char*)(span->_pageId << PAGE_SHIFT);
	size_t bytes = span->_n << PAGE_SHIFT;
	char* end = start + bytes;

	//������ڴ��з�Ϊһ��һ��С�ڴ棬�����ӵ�freeList��
	span->_freeList = start;
	start += size;
	void* tail = span->_freeList;
	while (start <end)
	{
		NextObj(tail) = start;
		tail = NextObj(tail);
		start += size;
	}
	NextObj(tail) = nullptr;
	span->_objSize = size;
	_spanLists[index].Mutex().lock();
	_spanLists[index].PushFront(span);
	return span;
}

void* CentralCache::GetLargeMem(size_t size)
{
	size_t alignSize = SizeClass::RoundUp(size);
	size_t kpage = alignSize >> PAGE_SHIFT;
	PageCache::GetInstance()->Mutex()->lock();
	Span* span = PageCache::GetInstance()->NewSpan(kpage);
	PageCache::GetInstance()->Mutex()->unlock();
	span->_objSize = alignSize;
	return (void*)(span->_pageId << PAGE_SHIFT);
}

void CentralCache::ReleaseLargeMem(void* ptr)
{
	PAGE_ID id = (PAGE_ID)ptr >> PAGE_SHIFT;
	Span* span = PageCache::GetInstance()->MapObjToSpan(ptr);
	size_t size = span->_objSize;
	if (size > MAX_BYTES)
	{
		PageCache::GetInstance()->Mutex()->lock();
		PageCache::GetInstance()->ReleaseSpan(span);
		PageCache::GetInstance()->Mutex()->unlock();
	}
}

//��ThreadCache�ͷŵ�С���ڴ����¹ҵ�Span�ϣ�����spanС���ڴ涼���黹ʱ�����ͷŵ�PageCache��
void CentralCache::ReleaseListToSpans(void* begin, size_t size)
{
	size_t index = SizeClass::Index(size);
	_spanLists[index].Mutex().lock();
	//��ÿ��С�ڴ�ͨ��ӳ���ҵ����Ӧ��span��������ȥ
	while (begin != nullptr)
	{
		void* next = NextObj(begin);
		Span* span = PageCache::GetInstance()->MapObjToSpan(begin);
		//ͷ����span��freeList��
		NextObj(begin) = span->_freeList;
		span->_freeList = begin;
		span->_useCount--;
		begin = next;
		//���span������С���ڴ��Ϊ��ʹ��������յ�PageCache��
		if (span->_useCount == 0)
		{
			_spanLists[index].Erase(span);
			_spanLists[index].Mutex().unlock();
			PageCache::GetInstance()->Mutex()->lock();
			PageCache::GetInstance()->ReleaseSpan(span);
			PageCache::GetInstance()->Mutex()->unlock();
			_spanLists[index].Mutex().lock();

		}
	}
	_spanLists[index].Mutex().unlock();
}


PageCache PageCache::_sInstPageCache;
Span* PageCache::NewSpan(size_t k)
{
	assert(k >= 1);
	if (k > KPAGE - 1)
	{
		void* ptr = SystemAlloc(k);
		//Span* span = new Span;
		Span* span = _spanPool.New();

		span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
		span->_n = k;

		//-------------���������������Ż�---------------------//
		//_idSpanMap[span->_pageId] = span;
		//_idSpanMap[span->_pageId] = span;
		_idSpanMap.set(span->_pageId, span);
		return span;
	}

	//����һ��Ͱ��û��Span
	if (!_pageLists[k].Empty())
	{
		Span* span = _pageLists[k].PopFront();
		//���ֳ���CentralCache��spanÿ��ҳ����ָ��ӳ������
		for (size_t i = 0; i < span->_n; i++)
		{
			//_idSpanMap[span->_pageId + i] = span;
			_idSpanMap.set(span->_pageId + i, span);
		}
		span->_isUse = true;
		return span;
	}

	for (size_t i = k + 1; i < KPAGE; i++)
	{
		//�����kpages���Ͱ����span��ֱ�ӷָ�span
		if (!_pageLists[i].Empty())
		{
			Span* span = _pageLists[i].PopFront();
			Span* newSpan = _spanPool.New();
			newSpan->_pageId = span->_pageId;
			newSpan->_n = k;

			span->_n -= k;
			span->_pageId += k;
			_pageLists[span->_n].PushFront(span);

			//�������PageCache�б����ҳ��ֻ��Ҫ������βҳ����ӳ�伴��
			//_idSpanMap[span->_pageId] = span;
			//_idSpanMap[span->_pageId + span->_n - 1] = span;

			_idSpanMap.set(span->_pageId, span);
			_idSpanMap.set(span->_pageId + span->_n - 1, span);
			//���ֳ���CentralCache��spanÿ��ҳ����ָ��ӳ������
			for (size_t i = 0; i < newSpan->_n; i++)
			{
				//_idSpanMap[newSpan->_pageId + i] = newSpan;
				_idSpanMap.set(newSpan->_pageId + i, newSpan);
			}

			return newSpan;
		}
	}
	//�����kpages���Ͱ��û��span������ϵͳ����

	//��ϵͳ������ڴ�������ַ��ֱ��ӳ��Ϊҳ��
	//ֻҪ��֤PAGE��ϵͳҳ����һ����С����

	void* ptr = SystemAlloc(KPAGE - 1);
	Span* span = _spanPool.New();
	span->_pageId = (PAGE_ID)ptr >> PAGE_SHIFT;
	span->_n = KPAGE - 1;
	_pageLists[span->_n].PushFront(span);
	return NewSpan(k);

}

//�������ַת��Ϊҳ�ţ�Ѱ�Ҷ�Ӧ��span
Span* PageCache::MapObjToSpan(void* obj)
{
	PAGE_ID id = (PAGE_ID)obj >> PAGE_SHIFT;
	auto ret = (Span*)_idSpanMap.get(id);
	assert(ret != nullptr);
	return ret;
}

//��CentralCache�����Ŀ���span��ҳ�����ڵ�span�ϲ�
void PageCache::ReleaseSpan(Span* span)
{
	//�Ƚ�С�ڴ�span��һҳ������ҳ�ϲ�
	while (1)
	{
		PAGE_ID id = span->_pageId;

		//�����Ż�
		auto ret = (Span*)_idSpanMap.get(id);
		if (ret == nullptr)
			break;

		Span* prevSpan = ret;
		if (prevSpan->_isUse == true)
			break;

		// �ϲ�������128ҳ��spanû�취�������ϲ���
		if (prevSpan->_n + span->_n > KPAGE - 1)
			break;

		//�ϲ��߼�
		span->_pageId = prevSpan->_pageId;
		span->_n += prevSpan->_n;

		_pageLists[prevSpan->_n].Erase(prevSpan);
		//delete prevSpan;
		_spanPool.Delete(prevSpan);
	}

	//���ڴ�spanҳ֮�������span�ϲ� ���ϲ�
	while (1)
	{

		PAGE_ID nextId = span->_pageId + span->_n;

		//-------------���������������Ż�---------------------//
		auto ret = (Span*)_idSpanMap.get(nextId);
		if (ret == nullptr)
		{
			break;
		}

		//Span* nextSpan = ret->second;
		Span* nextSpan = ret;
		if (nextSpan->_isUse == true)
			break;

		if (nextSpan->_n + span->_n > KPAGE - 1)
			break;

		span->_n += nextSpan->_n;
		_pageLists[nextSpan->_n].Erase(nextSpan);
		//delete nextSpan;
		_spanPool.Delete(nextSpan);


	}
	//�ϲ���Ҫ�޸�pageId��span����������Ȼ���ҵ�֮ǰ���ͷŵ�span
	_pageLists[span->_n].PushFront(span);
	span->_isUse = false;

	//-------------���������������Ż�---------------------//
	/*_idSpanMap[span->_pageId] = span;
	_idSpanMap[span->_pageId + span->_n - 1] = span;*/

	_idSpanMap.set(span->_pageId, span);
	_idSpanMap.set(span->_pageId + span->_n - 1, span);

}

