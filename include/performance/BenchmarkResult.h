#pragma once

#include <chrono>
#include <cstddef>
#include <string>

struct BenchmarkResult
{
    // Benchmark metadata
    std::string name;

    std::size_t operations = 0;
    std::size_t iterations = 1;

    // Last benchmark run (useful for debugging)
    std::chrono::nanoseconds duration{};

    double operationsPerSecond = 0.0;

    // Throughput statistics
   double averageOperationsPerSecond = 0.0;
double minimumOperationsPerSecond = 0.0;
double maximumOperationsPerSecond = 0.0;

double standardDeviation = 0.0;
double throughputVariationPercent = 0.0;

double averageRuntimeNs = 0.0;
double minimumRuntimeNs = 0.0;
double maximumRuntimeNs = 0.0;
double runtimeStandardDeviation = 0.0;

double averageLatencyNs = 0.0;
};