#pragma once

#include <string>
#include <vector>

#include "performance/BenchmarkResult.h"
#include "performance/Timer.h"

class BenchmarkRunner
{
public:
    explicit BenchmarkRunner(const std::string& benchmarkName);

    void start();
    void stop(std::size_t operations);
    void reset();

    BenchmarkResult result() const;

private:
    Timer timer;

    BenchmarkResult benchmarkResult;

    // One entry per measured iteration
    std::vector<double> throughputHistory;
    std::vector<double> runtimeHistory;
};