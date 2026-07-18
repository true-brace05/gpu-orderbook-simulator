#include "BenchmarkUtils.h"

#include <functional>
#include <iostream>

Microseconds measureExecutionTime(const std::function<void()>& task)
{
    auto start = Clock::now();

    task();

    auto end = Clock::now();

    return std::chrono::duration_cast<Microseconds>(end - start);
}

void printBenchmarkResult(
    const std::string& benchmarkName,
    int operations,
    Microseconds duration)
{
    double seconds = duration.count() / 1'000'000.0;

    double throughput =
        operations / seconds;

    std::cout
        << benchmarkName << '\n'
        << "Operations : " << operations << '\n'
        << "Time       : " << duration.count() << " us\n"
        << "Throughput : " << throughput << " ops/sec\n\n";
}