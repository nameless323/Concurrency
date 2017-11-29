#pragma once

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThredSafeQueueOnPtr
{
public:
    ThredSafeQueueOnPtr() = default;

    ThredSafeQueueOnPtr(const ThredSafeQueueOnPtr& other)
    {
        std::lock_guard<std::mutex> lk(other.m_mut);
        m_dataQueue = other.m_dataQueue;
    }

    void Push(T newValue)
    {
        std::shared_ptr<T> data(std::make_shared<T>(std::move(newValue)));
        std::lock_guard<std::mutex> lk(m_mut);
        m_dataQueue.push(data);
        m_dataCond.notify_one();
    }

    void WaitAndPop(T& value)
    {
        std::unique_lock<std::mutex> lk(m_mut);
        m_dataCond.wait(lk, [this] { return !m_dataQueue.empty(); });
        value = std::move(*m_dataQueue.front());
        m_dataQueue.pop();
    }

    std::shared_ptr<T> WaitAndPop()
    {
        std::unique_lock<std::mutex> lk(m_mut);
        m_dataCond.wait(lk, [this] { return !m_dataQueue.empty(); });
        std::shared_ptr<T> res = m_dataQueue.front();
        m_dataQueue.pop();
        return res;
    }

    bool TryPop(T& value)
    {
        std::lock_guard<std::mutex> lk(m_mut);
        if (m_dataQueue.empty())
            return false;
        value = std::move(*m_dataQueue.front());
        m_dataQueue.pop();
        return true;
    }

    std::shared_ptr<T> TryPop()
    {
        std::lock_guard<std::mutex> lk(m_mut);
        if (m_dataQueue.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res = m_dataQueue.front();
        m_dataQueue.pop();
        return res;
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lk(m_mut);
        return m_dataQueue.empty();
    }

private:
    mutable std::mutex m_mut;
    std::queue<std::shared_ptr<T>> m_dataQueue;
    std::condition_variable m_dataCond;
};