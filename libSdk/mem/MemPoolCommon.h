#pragma once
#include <cassert>
#include <mutex>
#include <memoryapi.h>
#pragma warning(disable:4334)
//�ڴ���������������ֽ���
static const size_t MAX_BYTES = 256 * 1024;
//ThreadCache��CentralCache��Ͱ�ĸ���
static const size_t FREELISTS_NUM = 208;

//PageCache��Ͱ��
static const size_t KPAGE = 129;
//һ��Page�Ĵ�С:2^13 -- ���ԸĽ�һ�£��Ȼ�ȡϵͳһҳ�Ĵ�С��Ȼ����ȷ��
static const size_t PAGE_SHIFT = 13;

//С���ڴ�ͷ4/8���ֽڴ洢ָ����һ��С���ڴ�
//�����ҵ���һ��С���ڴ�
inline  static void*& NextObj(void* obj)
{
	return *(void**)obj;
}

class FreeList {
public:
	void PushFront(void* obj)
	{
		NextObj(obj) = _freeList;
		_freeList = obj;
		_size++;
	}

	void PushRangeFront(void* begin, void* end, size_t n)
	{
		NextObj(end) = _freeList;
		_freeList = begin;
		_size += n;
	}

	void* PopFront()
	{
		void* obj = _freeList;
		_freeList = NextObj(_freeList);
		_size--;
		return obj;
	}

	void PopRangeFront(void*& begin, void*& end, size_t n)
	{
		assert(n <= _size);
		begin = _freeList;
		void* cur = _freeList;
		for (size_t i = 1; i < n; i++)
			cur = NextObj(cur);
		end = cur;
		NextObj(end) = nullptr;
		_freeList = NextObj(cur);
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
	void* _freeList = nullptr;
	size_t _maxSize = 1; //��centralcache�����ڴ������������������ȡС���ڴ���
	size_t _size = 0; //����ڵ����
};


class SizeClass {
public:
	//�������ڴ��ֽ�������ȡ��
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
			//�����ǳ���MAX_BYTES�����
			//����MAX_BYTES��ֱ�Ӱ�ҳ����
			return _RoundUp(size, 1 << PAGE_SHIFT);
		}
	}


	static size_t _Index(size_t size, size_t align_shift)
	{
		return ((size + (1 << align_shift) - 1) >> align_shift) - 1;
	}
	//�������ֽ�������ȡ�������õ���Ͱ�±�
	static size_t Index(size_t size)
	{
		assert(size <= MAX_BYTES);
		/*
		* threadCache��ÿ��Ͱ�������ڴ���С��һ
		* [1,128] ����16��Ͱ������Ͱ֮���ڴ��С���8B
		* [129,1024] ��56��Ͱ������Ͱ֮���ڴ��С���16B
		* [1025,8*1024] ��56��Ͱ������Ͱ֮���ڴ��С���128B
		* [8*1024+1,64*1024] ��56��Ͱ������Ͱ֮���ڴ��С���1024B
		* [64*1024+1,256*1024] ��24��Ͱ������Ͱ֮���ڴ��С���8*1024B
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

	//threadcacheһ����centralcacheҪ��obj����������
	static size_t NumMoveSize(size_t size)
	{
		assert(size > 0);
		//����������
		//С����һ��������������޸�
		//�����һ��������������޵�
		size_t num = MAX_BYTES / size;
		if (num < 2) num = 2;
		else if (num > 512) num = 512;
		return num;
	}
	//central cacheһ����page cache��Ҫ��span��ҳ��
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
	PAGE_ID _pageId = 0;//ҳ��
	size_t _n = 0;//ҳ��
	Span* _next = nullptr;	//ָ����һ��Span
	Span* _prev = nullptr;//ָ��ǰһ��Span
	void* _freeList = nullptr;//ָ��С��������ĵ�һ���ڵ㣬����һ��С����
	//----------------------------------------------------------
	//�������ֶΣ����ͷ��߼�����ϸ����
	size_t _objSize = 0;//����ÿ��С����Ĵ�С	
	size_t _useCount = 0;//�Ѿ��������thread cache��С�������
	bool _isUse = true;//���Span�Ƿ��page cache��ȡ��
};

class SpanList {
public:
	SpanList() {
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}

	~SpanList()//�ͷ������ÿ���ڵ�
	{
		Span* cur = _head->_next;
		while (cur != _head)
		{
			Span* next = cur->_next;
			delete cur;
			cur = next;
		}
		delete _head;
		_head = nullptr;
	}

	Span* Begin()//���ص�һ�����ݵ�ָ��
	{
		return _head->_next;
	}

	Span* End()//���һ������һ��ָ��
	{
		return _head;
	}

	void Insert(Span* pos, Span* newSapn) {
		assert(pos);
		assert(newSapn);

		Span* prev = pos->_prev;

		prev->_next = newSapn;
		newSapn->_prev = prev;

		newSapn->_next = pos;
		pos->_prev = newSapn;
	}

	void Erase(Span* pos) {
		assert(pos);
		assert(pos != _head);

		//�����ͷſռ�
		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
	}

	//β��
	void PushBack(Span* newspan)
	{
		Insert(End(), newspan);
	}

	//ͷ��
	void PushFront(Span* newspan)
	{
		Insert(Begin(), newspan);
	}


	std::mutex &Mutex() {
		return _mtx;
	}

	//βɾ
	Span* PopBack()//ʵ���ǽ�β��λ�õĽڵ��ó���
	{
		Span* span = _head->_prev;
		Erase(span);

		return span;
	}

	//ͷɾ
	Span* PopFront()//ʵ���ǽ�ͷ��λ�ýڵ��ó���
	{
		Span* span = _head->_next;
		Erase(span);

		return span;
	}

	void Lock()
	{
		_mtx.lock();
	}

	void Unlock()
	{
		_mtx.unlock();
	}

	bool Empty()
	{
		return _head->_next == _head;
	}
private:
	SpanList(const SpanList&) = delete;
	SpanList& operator=(const SpanList&) = delete;

	Span* _head;		//ͷ�ڵ�
	std::mutex _mtx;	//Ͱ��
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
	// sbrk unmmap��
	munmap(ptr,0);
#endif
}

#define SELF_MALLOC_ALIAS(self_fn) \
  __attribute__((alias(#self_fn), visibility("default")))
