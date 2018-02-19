#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <list>
#include <map>
#include <atomic>

template <typename T>
class LockFreeStackRefCount
{
public:
    ~LockFreeStackRefCount()
    {
        while (Pop());
    }

    void Push(const T& data)
    {
        CountedNodePtr newNode;
        newNode.ptr = new Node(data);
        newNode.ExternalCount = 1;
        newNode.ptr->Next = m_head.load();
        while (!m_head.compare_exchange_weak(newNode.ptr->Next, newNode))
        {
        }
    }



private:
    struct Node;

    struct CountedNodePtr
    {
        int ExternalCount = 0;
        Node* ptr = nullptr;
    };

    struct Node
    {
        std::shared_ptr<T> Data;
        std::atomic<int> InternalCount;
        CountedNodePtr Next;

        Node(const T& data)
            : Data(std::make_shared<T>(data))
            , InternalCount(0)
        {
        }
    };

    std::atomic<CountedNodePtr> m_head;


    void IncreaseHeadCount(CountedNodePtr& oldCounter)
    {
        CountedNodePtr newCounter;
        do 
        {
            newCounter = oldCounter;
            ++newCounter.ExternalCount;
        } while (!m_head.compare_exchange_strong(oldCounter, newCounter));
        oldCounter.ExternalCount = newCounter.ExternalCount;
    }
};