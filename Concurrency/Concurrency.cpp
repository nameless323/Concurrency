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
    std::thread t(Hello);
    t.join();
    return 0;
}

