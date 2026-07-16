#pragma once

#include <chrono>
#include <iostream>

inline void printBenchmarkResult(
    const std::string& benchmarkName,
    int numOperations,
    std::chrono::microseconds duration)
{
    const double seconds = duration.count() / 1'000'000.0;

    const double throughput =
        numOperations / seconds;

    std::cout << "=====================================\n";
    std::cout << benchmarkName << '\n';
    std::cout << "=====================================\n";

    std::cout << "Operations : "
              << numOperations << '\n';

    std::cout << "Time       : "
              << duration.count()
              << " us\n";

    std::cout << "Throughput : "
              << throughput
              << " ops/sec\n\n";
}