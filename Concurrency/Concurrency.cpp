#include "stdafx.h"

#include <iostream>
#include <thread>
#include <vector>

#include "Source/ParallelAccumulate.h"
#include "Source/ThreadSafeStack.h"
#include "Source/HierarchicalMutex.h"
#include "Source/ThreadSafeQueue.h"

void Hello()
{
    std::cout << " in multithreaded ";
    getchar();
}

int main()
{
    // unsigned int hc = std::thread::hardware_concurrency();
    // std::thread t(Hello);
    // t.join();
    ThredSafeQueue<int> q;
    HierarchicalMutex hkm(100);
    std::lock_guard<HierarchicalMutex> hm{ hkm };

    std::vector<int> v;
    for (int i = 0; i < 150; ++i)
    {
        v.push_back(i * i);
    }
    int res = ParallelAccumulate(v.begin(), v.end(), 0); ///std::accumulate(v.begin(), v.end(), 0);

    std::cout << res;

    getchar();
    return 0;
}

