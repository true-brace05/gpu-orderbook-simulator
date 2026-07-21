#include "performance/ReportPrinter.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{

std::string formatDuration(std::chrono::nanoseconds duration)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(2);

    const double ns = static_cast<double>(duration.count());

    if (ns >= 1'000'000'000.0)
    {
        out << ns / 1'000'000'000.0 << " s";
    }
    else if (ns >= 1'000'000.0)
    {
        out << ns / 1'000'000.0 << " ms";
    }
    else if (ns >= 1'000.0)
    {
        out << ns / 1'000.0 << " µs";
    }
    else
    {
        out << ns << " ns";
    }

    return out.str();
}

std::string formatThroughput(double opsPerSecond)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(2);

    if (opsPerSecond >= 1'000'000'000.0)
    {
        out << opsPerSecond / 1'000'000'000.0 << " B ops/sec";
    }
    else if (opsPerSecond >= 1'000'000.0)
    {
        out << opsPerSecond / 1'000'000.0 << " M ops/sec";
    }
    else if (opsPerSecond >= 1'000.0)
    {
        out << opsPerSecond / 1'000.0 << " K ops/sec";
    }
    else
    {
        out << opsPerSecond << " ops/sec";
    }

    return out.str();
}

} // namespace

void ReportPrinter::print(const BenchmarkResult& result)
{
    std::cout << "\n=========================================\n";

    std::cout << "Benchmark              : "
              << result.name << '\n';

    std::cout << "Operations             : "
              << result.operations << '\n';

    std::cout << "Iterations             : "
              << result.iterations << "\n\n";

    std::cout << "Average Runtime        : "
              << formatDuration(std::chrono::nanoseconds(
                     static_cast<long long>(result.averageRuntimeNs)))
              << '\n';

    std::cout << "Minimum Runtime        : "
              << formatDuration(std::chrono::nanoseconds(
                     static_cast<long long>(result.minimumRuntimeNs)))
              << '\n';

    std::cout << "Maximum Runtime        : "
              << formatDuration(std::chrono::nanoseconds(
                     static_cast<long long>(result.maximumRuntimeNs)))
              << '\n';

    std::cout << "Runtime Std. Dev.      : "
              << formatDuration(std::chrono::nanoseconds(
                     static_cast<long long>(result.runtimeStandardDeviation)))
              << "\n\n";

    std::cout << "Average Throughput     : "
              << formatThroughput(result.averageOperationsPerSecond)
              << '\n';

    std::cout << "Minimum Throughput     : "
              << formatThroughput(result.minimumOperationsPerSecond)
              << '\n';

    std::cout << "Maximum Throughput     : "
              << formatThroughput(result.maximumOperationsPerSecond)
              << '\n';

    std::cout << "Throughput Stability   : "
              << std::fixed << std::setprecision(2)
              << result.throughputVariationPercent
              << " %\n\n";

    std::cout << "Average Latency        : "
              << std::fixed << std::setprecision(2)
              << result.averageLatencyNs
              << " ns/op\n";

    std::cout << "=========================================\n";
}