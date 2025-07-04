#pragma once
void ConcurrentFree(void* ptr);
void* ConcurrentAllocate(size_t size);
void FreeThreadCache();
//void* operator new(size_t szMem);