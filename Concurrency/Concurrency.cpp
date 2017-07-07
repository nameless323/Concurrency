#include "stdafx.h"

#include <iostream>
#include <thread>
#include <vector>

#include "Source/ParallelAccumulate.h"

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

