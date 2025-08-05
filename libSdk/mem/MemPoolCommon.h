#pragma once
#include <cassert>
#include <mutex>
#ifdef _WIN32
#include <memoryapi.h>
#pragma warning(disable:4334)
#else
#include <sys/mman.h>
#include <exception>
#endif

static const size_t MAX_BYTES = 256 * 1024;

static const uint8_t FREELISTS_NUM = 208;

static const uint8_t KPAGE = 129;
static const uint8_t PAGE_SHIFT = 13;


inline  static void*& NextObj(void* obj)
{
	return *(void**)obj;
}

class FreeList {
public:
	void PushFront(void* obj)
	{
		NextObj(obj) = m_freeList;
		m_freeList = obj;
		_size++;
	}

	void PushRangeFront(void* begin, void* end, size_t n)
	{
		NextObj(end) = m_freeList;
		m_freeList = begin;
		_size += n;
	}

	void* PopFront()
	{
		void* obj = m_freeList;
		m_freeList = NextObj(m_freeList);
		_size--;
		return obj;
	}

	void PopRangeFront(void*& begin, void*& end, size_t n)
	{
		assert(n <= _size);
		begin = m_freeList;
		void* cur = m_freeList;
		for (size_t i = 1; i < n; i++)
			cur = NextObj(cur);
		end = cur;
		NextObj(end) = nullptr;
		m_freeList = NextObj(cur);
		_size -= n;
	}

	bool Empty()
	{
		return _size == 0;
	}

	size_t& MaxSize()
	{
		return _maxSize;
	}

	size_t Size()
	{
		return _size;
	}
private:
	void* m_freeList = nullptr;
	size_t _maxSize = 1; 
	size_t _size = 0; 
};


class SizeClass {
public:

	static size_t _RoundUp(size_t size, size_t align)
	{
		return  (size + align - 1) & ~(align - 1);
	}

	static size_t RoundUp(size_t size)
	{
		if (size <= 128) return _RoundUp(size, 8);
		else if (size <= 1024) return _RoundUp(size, 16);
		else if (size <= 8 * 1024) return _RoundUp(size, 128);
		else if (size <= 64 * 1024) return _RoundUp(size, 1024);
		else if (size <= MAX_BYTES) return _RoundUp(size, 8 * 1024);
		else {

			return _RoundUp(size, 1 << PAGE_SHIFT);
		}
	}


	static size_t _Index(size_t size, size_t align_shift)
	{
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}

	static size_t Index(size_t size)
	{
		assert(size <= MAX_BYTES);

		static const int group_array[] = { 16,56,56,56,24 };
		if (size <= 128) return _Index(size, 3);
		else if (size <= 1024) {
			return _Index(size - 128, 4) + group_array[0];
		}
		else if (size <= 8 * 1024) {
			return _Index(size - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (size <= 64 * 1024) {
			return _Index(size - 8 * 1024, 10) + group_array[2] + group_array[1] + group_array[0];
		}
		else if (size <= 256 * 1024) {
			return _Index(size - 64 * 1024, 13) + group_array[3] + group_array[2] + group_array[1] + group_array[0];
		}
		else assert(false);
		return -1;
	}

	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);
	
		size_t num = MAX_BYTES / size;
		if (num < 2) num = 2;
		else if (num > 512) num = 512;
		return num;
	}

	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);
		size_t pages = num * size;
		pages >>= PAGE_SHIFT;
		if (pages < 1) pages = 1;
		return pages;
	}
};
#ifdef _WIN64
typedef uint64_t PAGE_ID;
#elif _WIN32
typedef uint32_t PAGE_ID;
#else
typedef uint64_t PAGE_ID;
#endif
struct Span {
	PAGE_ID m_pageId = 0;
	size_t m_nPageNum = 0;
	Span* m_next = nullptr;	
	Span* m_prev = nullptr;
	void* m_freeList = nullptr;
	//----------------------------------------------------------
	
	size_t m_nObjSize = 0;
	size_t m_nUseCount = 0;
	bool m_isUse = true;
};

class SpanList {
public:
	SpanList() {
		m_headSpan = ::new Span;
		m_headSpan->m_next = m_headSpan;
		m_headSpan->m_prev = m_headSpan;
	}

	~SpanList()
	{
		Span* cur = m_headSpan->m_next;
		while (cur != m_headSpan)
		{
			Span* next = cur->m_next;
			delete cur;
			cur = next;
		}
		delete m_headSpan;
		m_headSpan = nullptr;
	}

	Span* Begin()
	{
		return m_headSpan->m_next;
	}

	Span* End()
	{
		return m_headSpan;
	}

	void Insert(Span* pos, Span* newSapn) {
		assert(pos);
		assert(newSapn);

		Span* prev = pos->m_prev;

		prev->m_next = newSapn;
		newSapn->m_prev = prev;

		newSapn->m_next = pos;
		pos->m_prev = newSapn;
	}

	void Erase(Span* pos) {
		assert(pos);
		assert(pos != m_headSpan);

		Span* prev = pos->m_prev;
		Span* next = pos->m_next;
		prev->m_next = next;
		next->m_prev = prev;
	}


	void PushBack(Span* newspan)
	{
		Insert(End(), newspan);
	}


	void PushFront(Span* newspan)
	{
		Insert(Begin(), newspan);
	}

	Span* PopBack()
	{
		Span* span = m_headSpan->m_prev;
		Erase(span);

		return span;
	}

	//ͷɾ
	Span* PopFront()
	{
		Span* span = m_headSpan->m_next;
		Erase(span);

		return span;
	}

	void Lock()
	{
		m_mtxSpan.lock();
	}

	void Unlock()
	{
		m_mtxSpan.unlock();
	}

	bool Empty()
	{
		return m_headSpan->m_next == m_headSpan;
	}
private:
	SpanList(const SpanList&) = delete;
	SpanList& operator=(const SpanList&) = delete;

	Span* m_headSpan;		
	std::mutex m_mtxSpan;	
};

inline void* SystemAlloc(size_t size)
{
	assert(size > 0);
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, size << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//linux brk
	void* ptr = mmap(NULL, size << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif
	if (ptr == nullptr)
	{
#ifdef _WIN32	
		printf("last error = %d", ::GetLastError());
#endif		
		throw std::bad_alloc();
	}

	return ptr;
}


inline void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	munmap(ptr,0);
#endif
}

#define SELF_MALLOC_ALIAS(self_fn) \
  __attribute__((alias(#self_fn), visibility("default")))
