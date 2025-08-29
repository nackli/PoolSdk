/***************************************************************************************************************************************************/
/*
* @Author: Nack Li
* @version 1.0
* @copyright 2025 nackli. All rights reserved.
* @License: MIT (https://opensource.org/licenses/MIT).
* @Date: 2025-08-29
* @LastEditTime: 2025-08-29
*/
/***************************************************************************************************************************************************/
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
