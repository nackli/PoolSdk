/*
 * @Author: nackli nackli@163.com
 * @Date: 2025-08-01 21:20:49
 * @LastEditors: nackli nackli@163.com
 * @LastEditTime: 2025-08-01 22:04:35
 * @FilePath: /PoolSdk/PoolSdk/PoolSdk.cpp
 * */
//
#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#include <unistd.h>
#endif
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <vector>
#include "FileLogger/FileLogger.h"
#include "Common/StringUtils.h"
#ifdef _WIN32
#pragma comment(lib,"libSdk.lib")
#endif

#ifndef _WIN32
unsigned long GetTickCount()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif


static void OnTestThread()
{
    while (1)
    {
        unsigned long dwTest = ::GetTickCount();
        for (int i = 0; i < 45000; i++)
        {
#if 0
            LOG_TRACE("Red-Black Tree after insertion: %d", i);
            LOG_DEBUG("Red-Black Tree after insertion:");
            LOG_INFO("Red-Black Tree after insertion:");
            LOG_WARN("Red-Black Tree after insertion:");
            LOG_ERROR("Red-Black Tree after insertion:");
            LOG_FATAL("Red-Black Tree after insertion:");
#else
            LOG_TRACE_S("Red-Black Tree after insertion: {}", i);
            LOG_DEBUG_S("Red-Black Tree after insertion:");
            LOG_INFO_S("Red-Black Tree after insertion:");
            LOG_WARN_S("Red-Black Tree after insertion:");
            LOG_ERROR_S("Red-Black Tree after insertion:");
            LOG_FATAL_S("Red-Black Tree after insertion:");
#endif
        }


        cout << "diff = " << " " << ::GetTickCount() - dwTest << endl;
    }
}

int main()
{
    FileLogger::getInstance().initLog("./logCfg.cfg");
    OnTestThread();



    while (1)
#ifdef _WIN32
        ::Sleep(10000000);
#else
        sleep(10000000);
#endif

    std::cout << "Hello World!\n";
}
