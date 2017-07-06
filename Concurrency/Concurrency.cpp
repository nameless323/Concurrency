#include "stdafx.h"

#include <iostream>
#include <thread>

void Hello()
{
    std::cout << " in multithreaded ";
    getchar();
}

int main()
{
    unsigned int hc = std::thread::hardware_concurrency();
    std::thread t(Hello);
    t.join();
    return 0;
}

