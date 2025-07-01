#pragma once
#ifdef _WIN32
#include <windows.h>
#define smp_rmb() MemoryBarrier()
#define smp_mb() MemoryBarrier()
#define smp_wmb() MemoryBarrier()
#else // 非Windows平台
#define smp_rmb() asm volatile("lfence" ::: "memory")	//lfence 读串行化
#define smp_mb() asm volatile("mfence" ::: "memory")	//mfence 读写都串行化
#define smp_wmb() asm volatile("sfence" ::: "memory")	//sfence 写串行化
#endif