// PoolSdk.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <Windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include "mem/Pool_Test.h"
#include "thread/AdvancedThreadPool.h"
#include "Common/FileLogger.h"
#include "Common/LockFreeCircularQue.h"
#include "Common/CircularQueue.h"
#include "Common/pipe.h"
#pragma comment(lib,"libSdk.lib")
int main()
{
    FileLogger::getInstance().initLog("./logCfg.cfg");
    AdvancedThreadPool pool(2, 4);
    PIPE_HANDLE hPipe[OPT_MAX];

    hPipe[OPT_WRITE] = createNamePipe("\\\\.\\pipe\\Test");

    hPipe[OPT_READ] = createNamePipe("\\\\.\\pipe\\Test");
    auto high_priority_task = pool.enqueue(
        AdvancedThreadPool::Priority::Normal,
        [&pool, hPipe]() {
            while (1)
            {
                auto threadId = std::this_thread::get_id();
                const char* szTestPipe = "hello nack";
                writePipe(hPipe[OPT_WRITE], szTestPipe,strlen(szTestPipe));
                LOG_DEBUG("High priority task running %d writePipe=%s", threadId, szTestPipe);
                //std::cout << " High priority task running " << threadId << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));;
            }

        }
    );

    auto high_priority_taskT = pool.enqueue(
        AdvancedThreadPool::Priority::Normal,
        [&pool, hPipe]() {
            while (1)
            {
                auto threadId = std::this_thread::get_id();
                char szData[256] = { 0 };
                readPipe(hPipe[OPT_READ], szData, sizeof(szData));
                LOG_DEBUG("High priority task running %d readPipe=%s", threadId, szData);
                //std::cout << " High priority task running " << threadId << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500));;
            }

        }
    );

    while (1)
    {
        size_t n = 1000;
        cout << "==========================================================" << endl;
        BenchmarkConcurrentMalloc(n, 100, 100);
        cout << endl << endl;

        BenchmarkMalloc(n, 100, 100);
        cout << "==========================================================" << endl;
    }


    system("pause");
    std::cout << "Hello World!\n";
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
