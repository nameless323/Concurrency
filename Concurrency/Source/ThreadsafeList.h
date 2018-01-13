#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <list>
#include <map>

template <typename T>
class ThreadsafeList
{
public:
    ThreadsafeList()
    {}

    ~ThreadsafeList()
    {
        RemoveIf([](const Node&) { return true; });
    }

    ThreadsafeList(const ThreadsafeList&) = delete;
    ThreadsafeList& operator=(const ThreadsafeList&) = delete;

    void PushFront(const T& value)
    {
        std::unique_ptr<Node> newNode(new Node(value));
        std::lock_guard<std::mutex> lk(m_head.M);
        newNode->Next = std::move(m_head.Next);
        m_head.Next = std::move(newNode);
    }

    template <typename Function>
    void ForEach(Function f)
    {
        Node* current = &m_head;
        std::unique_lock<std::mutex> nextLk(m_head.M);
        while (const Node* next = current->Next.get())
        {
            std::unique_lock<std::mutex> nextLk(next->M);
            lk.unlock();
            f(*next->Data);
            current = next;
            lk = std::move(nextLk);
        }
    }

    template <typename Pred>
    std::shared_ptr<T> FindFirstIf(Pred p)
    {
        Node* current = &m_head;
        std::unique_lock<std::mutex> lk(m_head.M);
        while (Node* const next = current->Next.get())
        {
            std::unique_lock<std::mutex> nextLk(next->M);
            lk.unlock();
            if (p(*next->Data))
                return next->Data;
            current = next;
            lk = std::move(nextLk);
        }
        return std::shared_ptr<T>();
    }

    template <typename Pred>
    void RemoveIf(Pred p)
    {
        Node* current = &m_head;
        std::unique_lock<std::mutex> lk(m_head.M);
        while (Node* const next = current->Next.get())
        {
            std::unique_lock<std::mutex> nextLk(next->M);
            if (p(*next->Data))
            {
                std::unique_ptr<Node> oldNext = std::move(current->Next);
                current->Next = std::move(next->Next);
                nextLk.unlock();
            }
            else
            {
                lk.unlock();
                current = next;
                lk = std::move(nextLk);
            }
        }
    }

private:
    struct Node
    {
        std::mutex M;
        std::shared_ptr<T> Data;
        std::unique_ptr<Node> Next;

        Node() : Next()
        {}

        Node(const T& value) 
            : data(std::make_shared<T>(value))
        {}
    };

    Node m_head;
};