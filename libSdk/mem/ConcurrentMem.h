#pragma once
#ifndef _CONCURRENT_MEM_H_
#define _CONCURRENT_MEM_H_
#ifdef MEM_POOL_OPEN
#define PM_MALLOC 			ConcurrentAllocate
#define PM_FREE 			ConcurrentFree
#else
#define PM_MALLOC 			malloc
#define PM_FREE 			free	
#endif

void ConcurrentFree(void* ptr);
void* ConcurrentAllocate(size_t size);
#endif

