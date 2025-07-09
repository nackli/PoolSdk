#pragma once
void ConcurrentFree(void* ptr);
void* ConcurrentAllocate(size_t size);
//void* operator new(size_t szMem);