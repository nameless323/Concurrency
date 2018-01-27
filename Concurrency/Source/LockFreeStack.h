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
        ++m_threadsInPop;
        Node* oldHead = m_head.load();
        while (oldHead && !m_head.compare_exchange_weak(oldHead, oldHead->Next));
        std::shared_ptr<T> res;
        if (oldHead)
        {
            res.swap(oldHead->Data);
        }
        TryReclaim(oldHead);
        return res;
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
    std::atomic<unsigned> m_threadsInPop;
    std::atomic<Node*> m_toBeDeleted;

    static void DeleteNodes(Node* nodes)
    {
        while (nodes)
        {
            Node* next = nodes->Next;
            delete nodes;
            nodes = next;
        }
    }

    void TryReclaim(Node* oldHead)
    {
        if (m_threadsInPop == 1)
        {
            Node* nodesToDelete = m_toBeDeleted.exchange(nullptr);
            if (!--m_threadsInPop)
            {
                DeleteNodes(nodesToDelete);
            }
            else if (nodesToDelete)
            {
                ChainPendingNodes(nodesToDelete);
            }
            delete oldHead;
        }
        else
        {
            ChainPendingNode(oldHead);
            --m_threadsInPop;
        }
    }

    void ChainPendingNodes(Node* nodes)
    {
        Node* last = nodes;
        while (Node* const next = last->Next)
        {
            last = next;
        }
        ChainPendingNodes(nodes, last);
    }

    void ChainPendingNodes(Node* first, Node* last)
    {
        last->Next = m_toBeDeleted;
        while (!m_toBeDeleted.compare_exchange_weak(last->Next, first))
        {
        };
    }

    void ChainPendingNode(Node* n)
    {
        ChainPendingNodes(n, n);
    }
};