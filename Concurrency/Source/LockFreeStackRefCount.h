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

    std::shared_ptr<T> Pop()
    {
        CountedNodePtr oldHead = m_head.load();
        for (;;)
        {
            IncreaseHeadCount(oldHead);
            Node* const ptr = oldHead.ptr;
            if (ptr == nullptr)
                return std::shared_ptr<T>();

            if (m_head.compare_exchange_strong(oldHead, ptr->Next))
            {
                std::shared_ptr<T> res;
                res.swap(ptr->Data);
                const int countIncrease = oldHead.ExternalCount - 2;
                if (ptr->InternalCount.fetch_add(countIncrease) == -countIncrease)
                {
                    delete ptr;
                }
                return res;
            }
            else if (ptr->InternalCount.fetch_sub(1) == 1)
            {
                delete ptr;
            }
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