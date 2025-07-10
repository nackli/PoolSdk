#pragma once
void ConcurrentFree(void* ptr);
void* ConcurrentAllocate(size_t size);
//void* operator new(size_t szMem, char emCold = 0)noexcept(false);
