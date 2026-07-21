#include "performance/BenchmarkRunner.h"

#include <algorithm>
#include <cmath>
#include <numeric>

BenchmarkRunner::BenchmarkRunner(const std::string& benchmarkName)
{
    benchmarkResult.name = benchmarkName;
}

void BenchmarkRunner::start()
{
    timer.start();
}

void BenchmarkRunner::stop(std::size_t operations)
{
    timer.stop();

    benchmarkResult.operations = operations;
    benchmarkResult.duration = timer.elapsed();

    runtimeHistory.push_back(
        static_cast<double>(benchmarkResult.duration.count()));

    const double seconds =
        benchmarkResult.duration.count() / 1'000'000'000.0;

    if (seconds > 0.0)
    {
        benchmarkResult.operationsPerSecond =
            operations / seconds;

        throughputHistory.push_back(
            benchmarkResult.operationsPerSecond);
    }
}

BenchmarkResult BenchmarkRunner::result() const
{
    BenchmarkResult result = benchmarkResult;

    if (!throughputHistory.empty() &&
        !runtimeHistory.empty())
    {
        // -------------------------------
        // Throughput statistics
        // -------------------------------

        const double total =
            std::accumulate(
                throughputHistory.begin(),
                throughputHistory.end(),
                0.0);

        result.averageOperationsPerSecond =
            total / throughputHistory.size();

        result.iterations =
            static_cast<int>(throughputHistory.size());

        result.minimumOperationsPerSecond =
            *std::min_element(
                throughputHistory.begin(),
                throughputHistory.end());

        result.maximumOperationsPerSecond =
            *std::max_element(
                throughputHistory.begin(),
                throughputHistory.end());

        double variance = 0.0;

        for (double throughput : throughputHistory)
        {
            const double diff =
                throughput -
                result.averageOperationsPerSecond;

            variance += diff * diff;
        }

        variance /= throughputHistory.size();

        result.standardDeviation =
            std::sqrt(variance);

        if (result.averageOperationsPerSecond > 0.0)
        {
            result.throughputVariationPercent =
                (result.standardDeviation /
                 result.averageOperationsPerSecond) * 100.0;
        }

        // -------------------------------
        // Runtime statistics
        // -------------------------------

        const double runtimeTotal =
            std::accumulate(
                runtimeHistory.begin(),
                runtimeHistory.end(),
                0.0);

        result.averageRuntimeNs =
            runtimeTotal / runtimeHistory.size();

        result.minimumRuntimeNs =
            *std::min_element(
                runtimeHistory.begin(),
                runtimeHistory.end());

        result.maximumRuntimeNs =
            *std::max_element(
                runtimeHistory.begin(),
                runtimeHistory.end());

        double runtimeVariance = 0.0;

        for (double runtime : runtimeHistory)
        {
            const double diff =
                runtime -
                result.averageRuntimeNs;

            runtimeVariance += diff * diff;
        }

        runtimeVariance /= runtimeHistory.size();

        result.runtimeStandardDeviation =
            std::sqrt(runtimeVariance);

        // -------------------------------
        // Latency
        // -------------------------------

        if (benchmarkResult.operations > 0)
        {
            result.averageLatencyNs =
                result.averageRuntimeNs /
                static_cast<double>(
                    benchmarkResult.operations);
        }
    }

    return result;
}

void BenchmarkRunner::reset()
{
    timer = Timer();

    const std::string benchmarkName =
        benchmarkResult.name;

    benchmarkResult = BenchmarkResult{};

    benchmarkResult.name = benchmarkName;
}