#include "BenchmarkUtils.h"
#include "OrderBook.h"
#include "workloads.h"
#include "performance/BenchmarkRunner.h"
#include "performance/ReportPrinter.h"

void benchmarkInsertion(int numOrders)
{
    BenchmarkRunner runner("Insertion Benchmark");

    // -----------------------------
    // Warm-up (NOT measured)
    // -----------------------------
    {
        OrderBook warmupBook;

        for (int j = 1; j <= numOrders; ++j)
        {
            warmupBook.addOrder({
                j,
                Side::Buy,
                OrderType::Limit,
                100,
                10,
                static_cast<uint64_t>(j)
            });
        }
    }

    constexpr int iterations = 5;

    for (int i = 0; i < iterations; ++i)
    {
        OrderBook book;

        runner.reset();

        runner.start();

        for (int j = 1; j <= numOrders; ++j)
        {
            book.addOrder({
                j,
                Side::Buy,
                OrderType::Limit,
                100,
                10,
                static_cast<uint64_t>(j)
            });
        }

        runner.stop(numOrders);
    }

    ReportPrinter::print(runner.result());
}

int main()
{
    for (int workload : workloads)
    {
        benchmarkInsertion(workload);
    }

    return 0;
}