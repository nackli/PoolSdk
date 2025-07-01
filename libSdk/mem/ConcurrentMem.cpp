#ifdef _WIN32
#include <Windows.h>
#endif
#include "ConcurrentMem.h"
#include "ThreadCache.h"
void* ConcurrentAllocate(size_t size)
{
	if (pTLSThreadCache == nullptr)
	{
		static ObjectMemPool<ThreadCache> threadPool;
		pTLSThreadCache = threadPool.New();
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