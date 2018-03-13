#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <list>
#include <map>
#include <atomic>
#include <future>
#include "ThreadSafeStack.h"

template <typename T>
struct Sorter
{
    struct ChunkToSort
    {
        std::list<T> Data;
        std::promise<std::list<T>> Promise;
    };

    ThreadSafeStack<ChunkToSort> Chunks;
    std::vector<std::thread> Threads;
    const unsigned int MaxThreadCount;
    std::atomic<bool> EndOfData;

    Sorter()
        : MaxThreadCount(std::thread::hardware_concurrency() - 1)
        , EndOfData(false)
    {}

    ~Sorter()
    {
        EndOfData = true;

        for (unsigned int i = 0; i < Threads.size(); ++i)
        {
            Threads[i].join();
        }
    }

    void TrySortChunk()
    {
        std::shared_ptr<ChunkToSort> chunk = Chunks.Pop();
        if (chunk != nullptr)
            SortChunk(chunk);
    }

    std::list<T> DoSort(std::list<T>& chunkData)
    {
        if (chunkData.empty())
            return chunkData;

        std::list<T> result;
        result.splice(result.begin(), chunkData, chunkData.begin());
        const T& partitionVal = *result.begin();

        typename std::list<T>::iterator dividePoint = std::partition(chunkData.begin(), chunkData.end(), [&](const T& val) { return val < partitionVal; });

        ChunkToSort newLowerChunk;
        newLowerChunk.Data.splice(newLowerChunk.Data.end(), chunkData, chunkData.begin(), dividePoint);

        std::future<std::list<T>> newLower = newLowerChunk.Promise.get_future();
        Chunks.Push(std::move(newLowerChunk));
        if (Threads.size() < MaxThreadCount)
            Threads.push_back(std::thread(&Sorter<T>::SortThread, this));

        std::list<T> newHigher(DoSort(chunkData));
        result.splice(result.end(), newHigher);
        while (newLower.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            TrySortChunk();

        result.splice(result.begin(), newLower.get());

        return result;
    }

    void SortChunk(const std::shared_ptr<ChunkToSort>& chunk)
    {
        chunk->Promise.set_value(DoSort(chunk->Data));
    }

    void SortThread()
    {
        while (!EndOfData)
        {
            TrySortChunk();
            std::this_thread::yield();
        }
    }
};

template <typename T>
std::list<T> ParallelQuickSort(std::list<T> input)
{
    if (input.empty())
        return input;
    Sorter<T> s;
    return s.DoSort(input);
}