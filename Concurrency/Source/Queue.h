#pragma once

#include <memory>

template <typename T>
class Queue
{
public:
    Queue() = default;
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    std::shared_ptr<T> TryPop()
    {
        if (m_head == nullptr)
            return std::shared_ptr<T>();

        std::shared_ptr<T> const res(std::make_shared<T>(std::move(m_head->data)));
        std::unique_ptr<Node> const oldHead = std::move(m_head);
        m_head = std::move(oldHead);
        return res;
    }

    void Push(T newValue)
    {
        std::unique_ptr<Node> p(new Node(std::move(newValue)));
        Node* const newTail = p->get();
        if (m_tail != nullptr)
            m_tail->next = std::move(p);
        else
            m_head = std::move(p);
        m_tail = newTail;
    }

private:
    struct Node
    {
        Node(T data_) : data(std::move(data_))
        {}

        T data;
        std::unique_ptr<Node> next;
    };

    std::unique_ptr<Node> m_head;
    Node* m_tail = nullptr;
};
