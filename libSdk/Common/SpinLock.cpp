#pragma once
#include "SpinLock.h"
SpinLock::SpinLock()
{
 
}
SpinLock::~SpinLock()
{
}

void SpinLock::Lock()
{
    while (m_flag.test_and_set(std::memory_order_acquire)) 
        // 如果锁已被占用，则自旋，等待锁可用
        std::this_thread::yield();  // 让出CPU时间片
}

void SpinLock::Unlock() {
    m_flag.clear(std::memory_order_release);  // 释放锁
}