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
#ifndef _PLATFORM_RW_LOCK_H_
#define _PLATFORM_RW_LOCK_H_
#include <mutex>
#ifndef _WIN32
#include <pthread.h>
#endif
#ifdef __cpp_lib_shared_mutex
#include <shared_mutex>
#endif

#include <condition_variable>
class RwLock
{
public:
    RwLock();
    ~RwLock();
    void readLock();
    void readUnlock();
    void writeLock();
    void writeUnlock();
private:
#ifdef _WIN32
#if _STL_LANG <= 201402L
    std::mutex m_mutex;
    std::condition_variable m_cvReadCond, m_cvWriteCond;
    int m_iReadCnt = 0;
    int m_iWritCnt = 0;
    int m_iWaitingWriters = 0;
    bool m_bWriteActive= false;
#else
    mutable std::shared_mutex m_mutex;
#endif
#else
    pthread_rwlock_t m_rwlock;
#endif
};
#endif

