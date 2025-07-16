#pragma once
#include <atomic>
#include <thread>
#include <chrono>

class SpinLock {

public:
    SpinLock();
    ~SpinLock();
    // ��ȡ��
    void Lock();
    // �ͷ���
    void Unlock();
private:
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT;  // ��ʼ��������־Ϊfalse
};