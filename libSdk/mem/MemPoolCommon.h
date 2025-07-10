#pragma once
#include <cassert>
#include <mutex>
#include <memoryapi.h>
#pragma warning(disable:4334)
//内存池最大申请可申请字节数
static const size_t MAX_BYTES = 256 * 1024;
//ThreadCache和CentralCache中桶的个数
static const size_t FREELISTS_NUM = 208;

//PageCache的桶数
static const size_t KPAGE = 129;
//一个Page的大小:2^13 -- 可以改进一下，先获取系统一页的大小，然后再确定
static const size_t PAGE_SHIFT = 13;

//小块内存头4/8个字节存储指向下一个小块内存
//用于找到下一个小块内存
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
	size_t _maxSize = 1; //向centralcache申请内存次数，用于慢启动获取小块内存数
	size_t _size = 0; //链表节点个数
};


class SizeClass {
public:
	//对所需内存字节数向上取整
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
			//这里是超过MAX_BYTES的情况
			//超过MAX_BYTES则直接按页对齐
			return _RoundUp(size, 1 << PAGE_SHIFT);
		}
	}


	static size_t _Index(size_t size, size_t align_shift)
	{
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}
	//对所需字节数向上取整，并得到其桶下标
	static size_t Index(size_t size)
	{
		assert(size <= MAX_BYTES);
		/*
		* threadCache中每个桶所含的内存块大小不一
		* [1,128] 中有16个桶，相邻桶之间内存大小相差8B
		* [129,1024] 有56个桶，相邻桶之间内存大小相差16B
		* [1025,8*1024] 有56个桶，相邻桶之间内存大小相差128B
		* [8*1024+1,64*1024] 有56个桶，相邻桶之间内存大小相差1024B
		* [64*1024+1,256*1024] 有24个桶，相邻桶之间内存大小相差8*1024B
		*/
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

	//threadcache一次向centralcache要的obj数量的上限
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);
		//慢启动策略
		//小对象一次批量申请的上限高
		//大对象一次批量申请的上限低
		size_t num = MAX_BYTES / size;
		if (num < 2) num = 2;
		else if (num > 512) num = 512;
		return num;
	}
	//central cache一次向page cache索要的span的页数
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
#elif LINUX
typedef uint64_t PAGE_ID;
#endif
struct Span {
	PAGE_ID m_pageId = 0;//页号
	size_t m_nPageNum = 0;//页数
	Span* m_next = nullptr;	//指向下一个Span
	Span* m_prev = nullptr;//指向前一个Span
	void* m_freeList = nullptr;//指向小对象单链表的第一个节点，串起一堆小对象
	//----------------------------------------------------------
	//这三个字段，在释放逻辑会详细讲解
	size_t m_nObjSize = 0;//所含每个小对象的大小	
	size_t m_nUseCount = 0;//已经被分配给thread cache的小对象个数
	bool m_isUse = true;//这个Span是否从page cache上取下
};

class SpanList {
public:
	SpanList() {
		m_headSpan = ::new Span;
		m_headSpan->m_next = m_headSpan;
		m_headSpan->m_prev = m_headSpan;
	}

	~SpanList()//释放链表的每个节点
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

	Span* Begin()//返回的一个数据的指针
	{
		return m_headSpan->m_next;
	}

	Span* End()//最后一个的下一个指针
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

		//不用释放空间
		Span* prev = pos->m_prev;
		Span* next = pos->m_next;
		prev->m_next = next;
		next->m_prev = prev;
	}

	//尾插
	void PushBack(Span* newspan)
	{
		Insert(End(), newspan);
	}

	//头插
	void PushFront(Span* newspan)
	{
		Insert(Begin(), newspan);
	}

	//尾删
	Span* PopBack()//实际是将尾部位置的节点拿出来
	{
		Span* span = m_headSpan->m_prev;
		Erase(span);

		return span;
	}

	//头删
	Span* PopFront()//实际是将头部位置节点拿出来
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

	Span* m_headSpan;		//头节点
	std::mutex m_mtxSpan;	//桶锁
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
		printf("last error = %d", ::GetLastError());
		throw std::bad_alloc();
	}

	return ptr;
}


inline void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr, 0, MEM_RELEASE);
#else
	// sbrk unmmap等
	munmap(ptr,0);
#endif
}

#define SELF_MALLOC_ALIAS(self_fn) \
  __attribute__((alias(#self_fn), visibility("default")))
