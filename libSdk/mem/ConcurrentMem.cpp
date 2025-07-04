#ifdef _WIN32
#include <Windows.h>
#endif
#include "ThreadCache.h"
#include "ConcurrentMem.h"
void* ConcurrentAllocate(size_t size)
{
	if (pTLSThreadCache == nullptr)
	{
		//static ObjectMemPool<ThreadCache> threadPool;
		pTLSThreadCache = new ThreadCache;
	}
	assert(pTLSThreadCache);
	return pTLSThreadCache->Allocate(size);
}

void ConcurrentFree(void* ptr)
{
	if (!ptr)
		return;
	assert(pTLSThreadCache);
	pTLSThreadCache->Deallocate(ptr);
}

void FreeThreadCache()
{
	delete pTLSThreadCache;
	pTLSThreadCache = nullptr;
}
//
//static void* operator new(size_t szMem)
//{
//	if (szMem == 0)
//		szMem = 1;
//	if (void* ptr = ConcurrentAllocate(szMem))
//		return ptr;
//	//throw std::bad_alloc{};
//	return nullptr;
//}
