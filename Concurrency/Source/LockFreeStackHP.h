#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <list>
#include <map>
#include <atomic>

const unsigned MaxHazardPointers = 100;
struct HazardPointer
{
    std::atomic<std::thread::id> id;
    std::atomic<void*> pointer;
};

HazardPointer HazardPointers[MaxHazardPointers];

bool OutstandingHazardPointersFor(void* p)
{
    for (unsigned i = 0; i < MaxHazardPointers; ++i)
    {
        if (HazardPointers[i].pointer.load() == p)
            return true;
    }
    return false;
}

class HpOwner
{
public:
    HpOwner(const HpOwner&) = delete;
    HpOwner operator= (const HpOwner&) = delete;
    HpOwner() : m_hp(nullptr)
    {
        for (unsigned i = 0; i < MaxHazardPointers; ++i)
        {
            std::thread::id oldId;
            if (HazardPointers[i].id.compare_exchange_strong(oldId, std::this_thread::get_id()))
            {
                m_hp = &HazardPointers[i];
                break;
            }
        }
        if (!m_hp)
            throw std::runtime_error("No hazard pointer available");
    }

    std::atomic<void*>& GetPointer()
    {
        return m_hp->pointer;
    }

    ~HpOwner()
    {
        m_hp->pointer.store(nullptr);
        m_hp->id.store(std::thread::id());
    }

private:
    HazardPointer* m_hp;
};

template <typename T>
void DoDelete(void* p)
{
    delete static_cast<T*>(p);
}

struct DataToReclaim
{
    void* Data;
    std::function<void(void*)> Deleter;
    DataToReclaim* Next;

    template <typename T>
    DataToReclaim(T* p)
        : Data(p)
        , Deleter(&DoDelete<T>)
        , Next(nullptr)
    {
    }

    ~DataToReclaim()
    {
        Deleter(Data);
    }
};

std::atomic<DataToReclaim*> NodesToReclaim;

void AddToReclaimList(DataToReclaim* node)
{
    node->Next = NodesToReclaim.load();
    while (!NodesToReclaim.compare_exchange_weak(node->Next, node));
}

template <typename T>
void ReclaimLater(T* data)
{
    AddToReclaimList(new DataToReclaim(data));
}

void DeleteNodesWithNoHazards()
{
    DataToReclaim* current = NodesToReclaim.exchange(nullptr);
    while (current)
    {
        DataToReclaim* const next = current->Next;
        if (OutstandingHazardPointersFor(current->Data))
            delete current;
        else
            AddToReclaimList(current);
        current = next;
    }
}

template <typename T>
class LockFreeStackHP
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
        std::atomic<void*>& hp = GetHazardPointerForCurrentThread();
        Node* oldHead = m_head.load();
        do
        {
            Node* tmp;
            do
            {
                tmp = oldHead;
                hp.store(oldHead);
                oldHead = m_head.load();
            } while (oldHead != tmp);
        } while (oldHead && !m_head.compare_exchange_strong(oldHead, oldHead->Next));
        hp.store(nullptr);
        std::shared_ptr<T> res;
        if (oldHead)
        {
            res->swap(oldHead->Data);
            if (OutstandingHazardPointersFor(oldHead))
                ReclaimLater(oldHead);
            else
                delete oldHead;
            DeleteNodesWithNoHazards();
        }
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
