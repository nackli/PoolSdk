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