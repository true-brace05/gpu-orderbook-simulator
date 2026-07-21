#pragma once

#include "performance/BenchmarkResult.h"

class ReportPrinter
{
public:
    static void print(const BenchmarkResult& result);
};