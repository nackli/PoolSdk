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
        // ������ѱ�ռ�ã����������ȴ�������
        std::this_thread::yield();  // �ó�CPUʱ��Ƭ
}

void SpinLock::Unlock() {
    m_flag.clear(std::memory_order_release);  // �ͷ���
}