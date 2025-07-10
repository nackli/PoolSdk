#ifdef _WIN32
#include <Windows.h>
#endif
#include "ThreadCache.h"
#include "ConcurrentMem.h"

void* ConcurrentAllocate(size_t size)
{
	return m_TLSThreadCache.Allocate(size);
}

void ConcurrentFree(void* ptr)
{
	if (!ptr)
		return;
	m_TLSThreadCache.Deallocate(ptr);
}

//
//void* operator new(size_t szMem,char emCold)
//{
//	if (szMem == 0)
//		szMem = 1;
//	if (void* ptr = ConcurrentAllocate(szMem))
//		return ptr;
//	throw std::bad_alloc{};
//	return nullptr;
//}
