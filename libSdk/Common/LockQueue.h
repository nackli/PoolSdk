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
    void setSpaces(size_t uCap) const { m_uCapSize = uCap; }
    bool empty()
    {
        //lock_type lock(m_mtxLock);
        return m_queueData.empty();
    }

    size_t size() const
    {
        //lock_type lock(m_mtxLock);
        return m_queueData.size();
    }

    void push(value_type const& value)
    {
        {
            lock_type lock(m_mtxLock);
            while (m_queueData.size() == m_uCapSize)
                m_cvWrite.wait(lock);
            m_queueData.emplace(value);
        }
        m_cvRead.notify_one();
    }

    void push(value_type&& value)
    {
        {
            lock_type lock(m_mtxLock);
            while (m_queueData.size() == m_uCapSize)
                m_cvWrite.wait(lock);
            m_queueData.emplace(value);
        }
        m_cvRead.notify_one();
    }

    value_type front()
    {
        value_type value;
        {
            lock_type lock(m_mtxLock);
            while (m_queueData.empty())
                m_cvRead.wait(lock);
            value = m_queueData.front();
        }
        return value;
    }

    value_type back()
    {
        value_type value;
        {
            lock_type lock(m_mtxLock);
            while (m_queueData.empty())
                m_cvRead.wait(lock);
            value = m_queueData.back();
        }
        return value;
    }

    value_type pop()
    {
        {
            lock_type lock(m_mtxLock);
            while (m_queueData.empty())
                m_cvRead.wait(lock);
            m_queueData.pop();
        }
        m_cvWrite.notify_one();
    }

    value_type pop_front()
    {
        value_type value;
        {
            lock_type lock(m_mtxLock);
            while (m_queueData.empty())
                m_cvRead.wait(lock);
            value = m_queueData.front();
            m_queueData.pop();
        }
        m_cvWrite.notify_one();
        return value;
    }

    void pop_back()
    {
        value_type value;
        {
            lock_type lock(m_mtxLock);
            while (m_queueData.empty())
                m_cvRead.wait(lock);
            value = m_queueData.back();
            m_queueData.pop();
        }
        m_cvWrite.notify_one();
        return value;
    }

private:
    std::queue<value_type> m_queueData;
    std::mutex m_mtxLock;
    std::condition_variable m_cvWrite;
    std::condition_variable m_cvRead;
    size_t m_uCapSize = 50;
};
#endif