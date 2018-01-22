#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <list>
#include <map>
#include <atomic>

template <typename T>
class LockFreeStack
{
public:
    void Push(const T& data)
    {
        Node* const newNode = new Node(data);
        newNode->Next = m_head.load();
        while (!m_head.compare_exchange_weak(newNode->Next, newNode));
    }

    std::shared_ptr<T> Pop()
    {
        Node* oldHead = m_head.load();
        while (oldHead && !m_head.compare_exchange_weak(oldHead, oldHead->Next));
        return oldHead ? oldHead->Data : std::shared_ptr<T>();
    }

private:
    struct Node
    {
        std::shared_ptr<T> Data;
        Node* Next;

        Node(const T& data)
            : Data(std::make_shared<T>(data))
        {}
    };

    std::atomic<Node> m_head;
};