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
        std::this_thread::yield();  
}

void SpinLock::Unlock() {
    m_flag.clear(std::memory_order_release); 
}