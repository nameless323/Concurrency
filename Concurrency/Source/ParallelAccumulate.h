#pragma once

#include <numeric>
#include <thread>

template <typename Iterator, typename T>
struct AccumulateBlock
{
    void operator()(Iterator begin, Iterator end, T& result)
    {
        result = std::accumulate(begin, end, result);
    }
};

template <typename Iterator, typename T>
T ParallelAccumulate(Iterator begin, Iterator end, T init)
{
    const size_t len = std::distance(begin, end);
    if (len == 0)
        return init;

    const size_t minPerThread = 25;
    const size_t maxThreads = (len + minPerThread - 1) / minPerThread;

    const size_t hardwareThreads = std::thread::hardware_concurrency();

    const size_t numThreads = std::min(hardwareThreads == 0 ? 2 : hardwareThreads, maxThreads);
    const size_t blockSize = len / numThreads;

    std::vector<T> results(numThreads);
    std::vector<thread> threads(numThreads - 1);

    Iterator blockStart = begin;
    for (size_t i = 0; i < numThreads - 1; ++i)
    {
        Iterator blockEnd = blockStart;
        std::advance(blockEnd, blockSize);
        threads[i] = std::thread(AccumulateBlock<Iterator, T>(), blockStart, blockEnd, std::ref(results[i]));
        blockStart = blockEnd;
    }
    AccumulateBlock(blockStart, end, results[numThreads - 1]);
    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));

    return std::acaccumulate(results.begin(), results.end(), init);
}