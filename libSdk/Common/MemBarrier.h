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
#pragma once
#ifdef _WIN32
#include <windows.h>
#define smp_rmb() MemoryBarrier()
#define smp_mb() MemoryBarrier()
#define smp_wmb() MemoryBarrier()
#else // ��Windowsƽ̨
#define smp_rmb() asm volatile("lfence" ::: "memory")	//lfence �����л�
#define smp_mb() asm volatile("mfence" ::: "memory")	//mfence ��д�����л�
#define smp_wmb() asm volatile("sfence" ::: "memory")	//sfence д���л�
#endif