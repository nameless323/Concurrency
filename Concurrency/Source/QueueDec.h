#pragma once

#include <memory>
#include <mutex>
#include <memory>

template <typename T>
class QueueDec
{
public:
    QueueDec() : m_head(new Node), m_tail(m_head->get())
    {}

    QueueDec(const QueueDec&) = delete;
    QueueDec& operator=(const QueueDec&) = delete;

    std::shared_ptr<T> WaitAndPop()
    {
        std::unique_ptr<Node> const oldHead = WaitPopHead();
        return oldHead->data;
    }

    void WaitAndPop(T& value)
    {
        std::unique_ptr<Node> const oldHead = WaitPopHead(value);
    }

    std::shared_ptr<T> TryPop()
    {
        std::unique_ptr<Node> oldHead = TryPopHead();
        return oldHead == nullptr ? std::shared_ptr<T>() : oldHead;
    }

    std::shared_ptr<T> TryPop(T& value)
    {
        std::unique_ptr<Node> const oldHead = TryPopHead(value);
        return oldHead;
    }

    bool Empty()
    {
        std::lock_guard<std::mutex> l(m_headMutex);
        return m_head->get() == GetTail();
    }

    void Push(T newValue)
    {
        std::shared_ptr<T> newData(std::make_shared<T>(std::move(newValue)));
        std::unique_ptr<Node> p(new Node());

        {
            std::lock_guard<std::mutex> l(m_tailMutex);
            Node* const newTail = p->get();
            std::lock_guard<std::mutex> l(m_tailMutex);
            m_tail->data = newData;
            m_tail->next = std::move(p);
            m_tail = newTail;
        }
        m_dataCond.notify_one();
    }

private:
    struct Node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<Node> next;
    };

    Node* GetTail()
    {
        std::lock_guard<std::mutex> l(m_tailMutex);
        return m_tail;
    }

    std::unique_ptr<Node> PopHead()
    {
        std::unique_ptr<Node> oldHead = std::move(m_head);
        m_head = std::move(oldHead->next);
        return oldHead;
    }

    std::unique_lock<std::mutex> WaitForData()
    {
        std::unique_lock<std::mutex> l(m_headMutex);
        m_dataCond.wait(l, [&] { return m_head->get != GetTail(); });
        return std::move(l);
    }

    std::unique_ptr<Node> WaitPopHead()
    {
        std::unique_ptr<std::mutex> l(WaitForData());
        return PopHead();
    }

    std::unique_ptr<Node> WaitPopHead(T& value)
    {
        std::unique_lock<std::mutex> l(WaitForData());
        value = std::move(*m_head->data);
        return PopHead();
    }

    std::unique_ptr<Node> TryPopHead()
    {
        std::lock_guard<std::mutex> hl(m_headMutex);
        if (m_head->get() == GetTail())
            return std::unique_ptr<Node>();
        return PopHead();
    }

    std::unique_ptr<Node> TryPopHead(T& value)
    {
        std::lock_guard<std::mutex> hl(m_headMutex);
        if (m_head->get() == GetTail())
            return std::unique_ptr<Node>();
        value = std::move(m_head->data());
        return PopHead();
    }

    std::unique_ptr<Node> m_head;
    Node* m_tail = nullptr;

    std::mutex m_headMutex;
    std::mutex m_tailMutex;
    std::condition_variable m_dataCond;
};
