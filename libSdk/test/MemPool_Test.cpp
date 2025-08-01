﻿#include <Windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include "../Common/FileLogger.h"
#include "../mem/ConcurrentMem.h"
using namespace std;
// ntimes 一轮申请和释放内存的次数
// rounds 轮次
// nworks 并发线程量
#pragma comment(lib,"libSdk.lib")
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
    std::vector<std::thread> vthread(nworks);
    std::atomic<size_t> malloc_costtime = 0;
    std::atomic<size_t> free_costtime = 0;

    for (size_t k = 0; k < nworks; ++k)
    {
        vthread[k] = std::thread([&, k]() {
            std::vector<void*> v;
            v.reserve(ntimes);

            for (size_t j = 0; j < rounds; ++j)
            {
                size_t begin1 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    //v.push_back(malloc(16));
                    v.push_back(malloc((16 + i) % 8192 + 1));
                }
                size_t end1 = clock();

                size_t begin2 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    free(v[i]);
                }
                size_t end2 = clock();
                v.clear();

                malloc_costtime += (end1 - begin1);
                free_costtime += (end2 - begin2);
            }
            });
    }

    for (auto& t : vthread)
    {
        t.join();
    }
    LOG_DEBUG("aaaaaaaaaaaaaaa");
    printf("%zu个线程并发执行%zu轮次，每轮次malloc %zu次: 花费：%zu ms\n",
        nworks, rounds, ntimes, malloc_costtime.load());

    printf("%zu个线程并发执行%zu轮次，每轮次free %zu次: 花费：%zu ms\n",
        nworks, rounds, ntimes, free_costtime.load());

    printf("%zu个线程并发malloc&free %zu次，总计花费：%zu ms\n",
        nworks, nworks * rounds * ntimes, malloc_costtime + free_costtime);
}


// 单轮次申请释放次数 线程数 轮次
void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
    std::vector<std::thread> vthread(nworks);
    std::atomic<size_t> malloc_costtime = 0;
    std::atomic<size_t> free_costtime = 0;
    static const size_t MAX_BYTES = 512 * 1024;
    for (size_t k = 0; k < nworks; ++k)
    {
        vthread[k] = std::thread([&]() 
         {
            std::vector<void*> v;
            v.reserve(ntimes);

            for (size_t j = 0; j < rounds; ++j)
            {
                size_t begin1 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    //v.push_back(ConcurrentAlloc(16));
                    v.push_back(ConcurrentAllocate((rand() + i) % MAX_BYTES + 1));
                }
                size_t end1 = clock();

                size_t begin2 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    ConcurrentFree(v[i]);
                }
                size_t end2 = clock();
                v.clear();

                malloc_costtime += (end1 - begin1);
                free_costtime += (end2 - begin2);
            }
         });
    }

    for (auto& t : vthread)
    {
        t.join();
    }
    LOG_DEBUG("bbbbbbbbbbbbb");
    printf("%zu个线程并发执行%zu轮次，每轮次concurrent alloc %zu次: 花费：%zu ms\n",
        nworks, rounds, ntimes, malloc_costtime.load());

    printf("%zu个线程并发执行%zu轮次，每轮次concurrent dealloc %zu次: 花费：%zu ms\n",
        nworks, rounds, ntimes, free_costtime.load());

    printf("%zu个线程并发concurrent alloc&dealloc %zu次，总计花费：%zu ms\n",
        nworks, nworks * rounds * ntimes, malloc_costtime + free_costtime);
}