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
#include "Common/FileLogger.h"
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

int main()
{
    FileLogger::getInstance().initLog("./logCfg.cfg");
    while (1)
    {  
        unsigned long dwTest = ::GetTickCount();    
        for (int i = 1; i < 30000; i++)
        {
            LOG_TRACE("Red-Black Tree after insertion: %i",i);
            LOG_DEBUG("Red-Black Tree after insertion:");
            LOG_INFO("Red-Black Tree after insertion:");
            LOG_WARNING("Red-Black Tree after insertion:");
            LOG_ERROR("Red-Black Tree after insertion:");
            LOG_FATAL("Red-Black Tree after insertion:");
        }
        cout << "diff = " << ::GetTickCount() - dwTest << endl;
    }
    system("pause");
    std::cout << "Hello World!\n";
}
