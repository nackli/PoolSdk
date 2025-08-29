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
#include <atomic>
#include <thread>
#include <chrono>

class SpinLock {

public:
    SpinLock();
    ~SpinLock();
    void Lock();
    void Unlock();
private:
    std::atomic_flag m_flag = ATOMIC_FLAG_INIT; 
};