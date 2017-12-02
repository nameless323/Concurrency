#pragma once

#include <exception>
#include <memory>
#include <mutex>
#include <stack>

struct EmptyStackException : std::exception
{
    const char* what() const override 
    {
        return "The stack is empty";
    }
};

template <typename T>
class ThreadSafeStack
{
public:
    ThreadSafeStack() = default;
    const ThreadSafeStack(const ThreadSafeStack& other)
    {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_data = other.m_data;
    }
    ThreadSafeStack& operator= (const ThreadSafeStack&) = delete;

    void Push(T newValue)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_data.push(newValue);
    }

    std::shared_ptr<T> Pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_data.empty())
            throw EmptyStackException();
        std::shared_ptr<T> const res(std::make_shared<T>(m_data.top()));
        m_data.pop();
        return res;
    }

    void Pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_data.empty())
            throw EmptyStackException();
        value = m_data.top();
        m_data.pop();
    }

    bool Empty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_data.empty();
    }

private:
    std::stack<T> m_data;
    mutable std::mutex m_mutex;
};