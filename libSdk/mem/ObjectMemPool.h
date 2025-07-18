#pragma once
#include "MemPoolCommon.h"
#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif
template<class T>
class ObjectMemPool {
public:
	T* New()
	{
		T* obj = nullptr;
		if (m_freeList != nullptr)
		{
			obj = (T*)m_freeList;
			m_freeList = NextObj(m_freeList);
		}
		else
		{
			if (_remainSize < sizeof(T))
			{
				size_t allocSize = MAX(sizeof(T), (size_t)128 * 1024);
				_mem = (char*)SystemAlloc(allocSize >> PAGE_SHIFT);
				if (_mem == nullptr)
					throw std::bad_alloc();

				_remainSize = allocSize;
			}
			size_t size = MAX(sizeof(T), sizeof(void*));
			_remainSize -= size;
			obj = (T*)_mem;
			_mem += size;
		}
		::new(obj)T;
		return obj;
	}
	void Delete(T* obj)
	{
		obj->~T();
		NextObj(obj) = m_freeList;
		m_freeList = obj;
	}

private:
	char* _mem = nullptr;
	size_t _remainSize = 0;
	void* m_freeList = nullptr;
};

