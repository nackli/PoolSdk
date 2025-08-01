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
#endif
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include "Common/FileLogger.h"
#ifdef _WIN32
#pragma comment(lib,"libSdk.lib")
#endif

int main()
{
    FileLogger::getInstance().initLog("./logCfg.cfg");
    while (1)
    {
#ifdef _WIN32        
        DWORD dwTest = ::GetTickCount();
#else
        time_t  dwTest = time(nullptr);
#endif        
        for (int i = 1; i < 30000; i++)
        {
            LOG_TRACE("Red-Black Tree after insertion: %i",i);
            LOG_DEBUG("Red-Black Tree after insertion:");
            LOG_INFO("Red-Black Tree after insertion:");
            LOG_WARNING("Red-Black Tree after insertion:");
            LOG_ERROR("Red-Black Tree after insertion:");
            LOG_FATAL("Red-Black Tree after insertion:");
        }
#ifdef _WIN32 
        cout << "diff = " << ::GetTickCount() - dwTest << endl;
#else
        cout << "diff = " << time(nullptr) - dwTest << endl;
#endif
    }
    system("pause");
    std::cout << "Hello World!\n";
}
