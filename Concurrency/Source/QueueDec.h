#pragma once

#include <memory>

template <typename T>
class QueueDec
{
public:
    QueueDec() : m_head(new Node), m_head(m_head->get())
    {}

    QueueDec(const QueueDec&) = delete;
    QueueDec& operator=(const QueueDec&) = delete;

    std::shared_ptr<T> TryPop()
    {
        if (m_head->get() == m_tail)
            return std::shared_ptr<T>();

        std::shared_ptr<T> const res(m_head->data);
        std::unique_ptr<Node> const oldHead = std::move(m_head);
        m_head = std::move(oldHead->next);
        return res;
    }

    void Push(T newValue)
    {
        std::shared_ptr<T> newData(std::make_shared<T>(std::move(newValue)));
        std::unique_ptr<Node> p(new Node());
        m_tail->data = newData;
        Node* const newTail = p->get();
        m_tail = newTail;
    }

private:
    struct Node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<Node> next;
    };

    std::unique_ptr<Node> m_head;
    Node* m_tail = nullptr;
};
