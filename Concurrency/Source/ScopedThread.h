#pragma once

#include <stdexcept>
#include <thread>

class ScopedThread
{
public:
    ScopedThread(std::thread thread) : m_thread(std::move(thread))
    {
        if (!m_thread.joinable())
            throw std::logic_error("thread is not joinable");
    }
    ~ScopedThread()
    {
        m_thread.join();
    }

    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator= (const ScopedThread&) = delete;

private:
    std::thread m_thread;
};
