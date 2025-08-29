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
#include "RwLock.h"
#if _STL_LANG <= 201402L
#include <thread>
#include <shared_mutex>
#endif

RwLock::RwLock()
{
#ifndef _WIN32
    pthread_rwlock_init(&m_rwlock, nullptr);
#endif
}

RwLock::~RwLock()
{
#ifndef _WIN32
    pthread_rwlock_destroy(&m_rwlock);
#endif
}

void RwLock::readLock()
{
#ifdef _WIN32
#if _STL_LANG <= 201402L
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_iWritCnt > 0 || m_iWaitingWriters > 0) {
        m_cvReadCond.wait(lock);
    }
    m_iReadCnt++;
#else
    std::shared_lock lock(m_mutex);
#endif
#else
    pthread_rwlock_rdlock(&m_rwlock);
#endif
}

void RwLock::readUnlock()
{
#ifdef _WIN32
#if _STL_LANG <= 201402L
    std::unique_lock<std::mutex> lock(m_mutex);
    m_iReadCnt--;
    if (m_iReadCnt == 0 && m_iWaitingWriters > 0) {
        m_cvWriteCond.notify_one();
    }
#endif
#else
    pthread_rwlock_unlock(&m_rwlock);
#endif
}

void RwLock::writeLock()
{
#ifdef _WIN32
#if _STL_LANG <= 201402L
    std::unique_lock<std::mutex> lock(m_mutex);
    m_iWaitingWriters++;
    while (m_iReadCnt > 0 || m_iWritCnt > 0) {
        m_cvWriteCond.wait(lock);
    }
    m_iWaitingWriters--;
    m_iWritCnt++;
    m_bWriteActive = true;
#else
    std::unique_lock lock(m_mutex);  
#endif
#else
    pthread_rwlock_wrlock(&m_rwlock);
#endif
}

void RwLock::writeUnlock()
{
#ifdef _WIN32
#if _STL_LANG <= 201402L
    std::unique_lock<std::mutex> lock(m_mutex);
    m_iWritCnt--;
    m_bWriteActive = false;
    if (m_iWaitingWriters > 0) {
        m_cvWriteCond.notify_one();
    }
    else {
        m_cvReadCond.notify_all();
    }
#endif
#else
    pthread_rwlock_unlock(&m_rwlock);
#endif
}