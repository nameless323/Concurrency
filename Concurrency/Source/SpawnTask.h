#pragma once

#include <future>
#include <thread>

template<typename T, typename A>
std::future<std::result_of<F(A&&)>::type> SpawnTask(F&& f, A&& a)
{
    using resultType = std::result_of<F(A&&)>::type;
    std::packaged_task<resultType(A&&)> task(std::move(f));
    std::future<resultType> res(task.getFuture());
    std::thread t(std::move(task), std::move(a));
    t.detach();
    return res;
}
