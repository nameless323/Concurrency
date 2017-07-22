#pragma once

#include <algorithm>
#include <future>
#include <list>

template <typename T>
std::list<T> ParallelQuicksort(std::list<T> input)
{
    if (input.empty())
        return input;

    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin();

    auto dividePointIt = std::partition(input.begin(), input.end(), [&](const T& t) { return t < pivot; });
    std::list<T> lowerPart;
    lowerPart.splice(lowerPart.end(), input, input.begin(), dividePointIt);

    std::future<std::list<T>> newLower(std::async(&ParallelQuicksort(std::move(lowerPart))));
    auto newHigher(ParallelQuicksort(std::move(input)));
    result.splice(result.end(), newHigher);
    result.splice(result.begin(), newLower.get());

    return result;
}