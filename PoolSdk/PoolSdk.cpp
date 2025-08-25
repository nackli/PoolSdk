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
#include <list>
#ifdef _WIN32
#pragma comment(lib,"libSdk.lib")
#endif


int main()
{
    while (1)
#ifdef _WIN32
        ::Sleep(10000000);
#else
        sleep(10000000);
#endif

    std::cout << "Hello World!\n";
}
