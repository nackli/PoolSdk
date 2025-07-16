#pragma once
#include <atomic>
#include <thread>
#include <chrono>

class SpinLock {

public:
    SpinLock();
    ~SpinLock();
    // 获取锁
    void Lock();
    // 释放锁
    void Unlock();
private:
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;  // 初始化自旋标志为false
};