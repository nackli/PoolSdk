/***************************************************************************************************************************************************/
/*
* @Author: Nack Li
* @version 1.0
* @copyright 2025 nackli. All rights reserved.
* @License: MIT (https://opensource.org/licenses/MIT).
* @Date: 2025-08-29
 * @LastEditTime: 2026-09-06 09:39:01
*/
/***************************************************************************************************************************************************/
#ifndef _PLATFORM_CIRCULAR_QUEUE_H_
#define _PLATFORM_CIRCULAR_QUEUE_H_
#include <mutex>
#include <condition_variable>
#include <queue>

template<typename T>
class LockQueue
{
public:
    typedef T           value_type;
    typedef typename std::unique_lock<std::mutex> lock_type;
    LockQueue() {}
    LockQueue(LockQueue const&) = delete;
    LockQueue(LockQueue&&) = delete;
    LockQueue& operator = (LockQueue const&) = delete;
    LockQueue& operator = (LockQueue&&) = delete;

    size_t getSpaces() const { return m_uCapSize; }
    void setSpaces(size_t uCap) { m_uCapSize = uCap; }
    bool empty()
    {
        lock_type lock(m_mtxLock);
        return m_queueData.empty();
    }

    size_t size()
    {
        lock_type lock(m_mtxLock);
        return m_queueData.size();
    }

    void push(value_type const& value)
    {
        {
            lock_type lock(m_mtxLock);
            m_cvWrite.wait(lock, [this]() {return m_queueData.size() != m_uCapSize || m_bStop; });
            if (m_bStop)
                return;                 // 队列已停止，丢弃新数据，避免永久阻塞
            m_queueData.emplace(std::move(value));
        }
        m_cvRead.notify_one();
    }

    void push(value_type&& value)
    {
        {
            lock_type lock(m_mtxLock);
            m_cvWrite.wait(lock, [this]() {return m_queueData.size() != m_uCapSize || m_bStop; });
            if (m_bStop)
                return;
            m_queueData.emplace(value);
        }
        m_cvRead.notify_one();
    }

    value_type front()
    {
        value_type value;
        {
            lock_type lock(m_mtxLock);
            m_cvRead.wait(lock, [this]() {return !m_queueData.empty() || m_bStop; });
            if (!m_queueData.empty())
                value = std::move(m_queueData.front());
        }
        return value;
    }

    value_type back()
    {
        value_type value;
        {
            lock_type lock(m_mtxLock);
            m_cvRead.wait(lock, [this]() {return !m_queueData.empty() || m_bStop; });
            if (!m_queueData.empty())
                value = m_queueData.back();
        }
        return value;
    }

    void pop()
    {
        {
            lock_type lock(m_mtxLock);    
            m_cvRead.wait(lock, [this]() {return !m_queueData.empty() || m_bStop; });
            if (!m_queueData.empty())
                m_queueData.pop();
        }
        m_cvWrite.notify_one();
    }

    value_type pop_front()
    {
        value_type value;
        {
            lock_type lock(m_mtxLock);
            m_cvRead.wait(lock, [this]() {return !m_queueData.empty() || m_bStop; });
            if (!m_queueData.empty())
            {
                value = std::move(m_queueData.front());
                // value = m_queueData.front();
                m_queueData.pop();
            }
        }
        m_cvWrite.notify_one();
        return value;
    }

    void pop_back()
    {
        value_type value;
        {
            lock_type lock(m_mtxLock);
            m_cvRead.wait(lock, [this]() {return !m_queueData.empty() || m_bStop; });
            if (!m_queueData.empty())
            {
                value = std::move(m_queueData.back());
                m_queueData.pop();
            }
        }
        m_cvWrite.notify_one();
        return value;
    }

    /* 停止并唤醒所有等待者：wakeup 之后所有 pop/push 立即返回，不再阻塞。
       仅供析构/停机场景使用，普通运行期不要调用。 */
    void wakeup()
    {
        {
            std::lock_guard<std::mutex> lock(m_mtxLock);
            m_bStop = true;
        }
        m_cvWrite.notify_all();
        m_cvRead.notify_all();    
    }
private:
    std::queue<value_type> m_queueData;
    std::mutex m_mtxLock;
    std::condition_variable m_cvWrite;
    std::condition_variable m_cvRead;
    bool m_bStop = false;               // wakeup 置位后所有等待立即返回
    size_t m_uCapSize = 100;
};
#endif