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

struct Date {
    int year, month, day, hour, min, sec, ms;
};

//template <>
//class fmt::formatter<Date> {
//private:
//    char presentation = 'Y';
//public:
//    template <typename ParseContext>
//    auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
//        auto it = ctx.begin();
//        auto end = ctx.end();
//        if (it != end && (*it == 'Y' || *it == 'D')) {
//            presentation = *it++;
//        }
//        if (it != end && *it != '}') {
//            throw fmt::format_error("invalid date format");
//        }
//        return it;
//    }
//public:
//    template <typename FormatContext>
//    auto format(const Date& d, FormatContext& ctx) const -> decltype(ctx.out()) {
//        if (presentation == 'Y') {
//            return fmt::format_to(ctx.out(), "{}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}.{:03d}", d.year, d.month, d.day,d.hour,d.min,d.sec,d.ms);
//        }
//        return fmt::format_to(ctx.out(), "{:02d}/{:02d}/{} {:02d}:{:02d}:{:02d}.{:03d}", d.day, d.month, d.year,  d.hour, d.min, d.sec, d.ms);
//    }
//};



static void OnTestThread()
{
    //int a = 1234;
    //string strTest = "afafdadfgagva";
    //Date d{ 2023, 4, 15 ,23,12,56,123};
    //string str = fmt::format("Default format: {},{},{}", a, strTest, d);
    //fmt::format<Point>("Test {}",pt);
    while (1)
    {
        unsigned long dwTest = ::GetTickCount();
        for (int i = 0; i < 45000; i++)
        {
#if 1
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

    //nklog::FormatterBuilder myTest("[%D] [tid:%t] [%l] [%@] %v%n%T");
    //nklog::LogMessage testLog(3, __FILE__, __LINE__, "msg-test-data-haha", __FUNCTION__);
    //string str = myTest.format(testLog);
    //fmt::memory_buffer tst;
    //fmt::format_to(std::back_inserter(tst), "The answer is {}.", 42);
    //tst.append(string("aaaaaaaaaa"));
    //string str1 = string(tst.data(), tst.size());
    //tst.append(buf_ptr, buf_ptr + view.size());
    OnTestThread();



    while (1)
#ifdef _WIN32
        ::Sleep(10000000);
#else
        sleep(10000000);
#endif

    std::cout << "Hello World!\n";
}
