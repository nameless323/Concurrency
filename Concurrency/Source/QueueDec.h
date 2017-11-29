#pragma once

#include <memory>
#include <mutex>

template <typename T>
class QueueDec
{
public:
    QueueDec() : m_head(new Node), m_tail(m_head->get())
    {}

    QueueDec(const QueueDec&) = delete;
    QueueDec& operator=(const QueueDec&) = delete;


    std::shared_ptr<T> TryPop()
    {
        std::unique_ptr<Node> oldHead = PopHead();
        return oldHead == nullptr ? shared_ptr<T>() : oldHead;
    }

    void Push(T newValue)
    {
        std::shared_ptr<T> newData(std::make_shared<T>(std::move(newValue)));
        std::unique_ptr<Node> p(new Node());
        Node* const newTail = p->get();
        std::lock_guard<std::mutex> l(m_tailMutex);
        m_tail->data = newData;
        m_tail->next = std::move(p);
        m_tail = newTail;
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
        std::lock_guard<std::mutex> l(m_headMutex);
        if (m_head->get() == GetTail())
            return nullptr;
        std::unique_ptr<Node> oldHead = std::move(m_head);
        m_head = std::move(oldHead->next);
        return oldHead;
    }

    std::unique_ptr<Node> m_head;
    Node* m_tail = nullptr;

    std::mutex m_headMutex;
    std::mutex m_tailMutex;
};
