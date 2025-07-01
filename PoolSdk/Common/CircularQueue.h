#ifndef _PLATFORM_CIRCULAR_QUEUE_H_
#define _PLATFORM_CIRCULAR_QUEUE_H_
#include <mutex>
#include <condition_variable>
template<typename T, std::size_t Cap = 10>
class CircularQueue
{
public:
    typedef T           value_type;
    typedef std::size_t size_type;
    typedef std::size_t pos_type;
    typedef typename std::lock_guard<std::mutex> lock_type;
    CircularQueue() { static_assert(Cap != 0); }
    CircularQueue(CircularQueue const&) = delete;
    CircularQueue(CircularQueue&&) = delete;
    CircularQueue& operator = (CircularQueue const&) = delete;
    CircularQueue& operator = (CircularQueue&&) = delete;

    size_type spaces() const { return Cap; }
    bool empty() const
    {
        lock_type lock(m_mutex);
        return m_iReadPos == m_iWritePos;
    }

    size_type size() const
    {
        lock_type lock(m_mutex);
        return Cap - m_iSpaceSize;
    }

    void push(value_type const& value)
    {
        {
            lock_type lock(m_mutex);
            while (!m_iSpaceSize)
                m_cvWrite.wait(lock);

            m_queueData[m_iWritePos] = value;
            --m_iSpaceSize;
            m_iWritePos = next_pos(m_iWritePos);
        }
        m_cvRead.notify_one();
    }

    void push(value_type&& value)
    {
        {
            lock_type lock(m_mutex);
            while (!m_iSpaceSize)
                m_cvWrite.wait(lock);

            m_queueData[m_iWritePos] = std::move(value);
            --m_iSpaceSize;
            m_iWritePos = next_pos(m_iWritePos);
        }
        m_cvRead.notify_one();
    }

    value_type pop()
    {
        value_type value;
        {
            lock_type lock(m_mutex);
            while (Cap == m_iSpaceSize)
                m_cvRead.wait(lock);

            value = std::move(m_queueData[m_iReadPos]);
            ++m_iSpaceSize;
            m_iReadPos = next_pos(m_iReadPos);
        }
        m_cvWrite.notify_one();
        return value;
    }

private:
    pos_type next_pos(pos_type pos) { return (pos + 1) % Cap; }
private:
    value_type m_queueData[Cap];
    pos_type m_iReadPos = 0;
    pos_type m_iWritePos = 0;
    size_type m_iSpaceSize = Cap;
    std::mutex m_mutex;
    std::condition_variable m_cvWrite;
    std::condition_variable m_cvRead;
};
#endif