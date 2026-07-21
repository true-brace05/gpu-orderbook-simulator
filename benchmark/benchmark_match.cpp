#include "performance/BenchmarkRunner.h"
#include "performance/ReportPrinter.h"

#include "OrderBook.h"
#include "workloads.h"

void benchmarkMatching(int numOrders)
{
    BenchmarkRunner runner("Matching Benchmark");

    // ---------------------------------------
    // Warm-up (NOT measured)
    // ---------------------------------------
    {
        OrderBook warmupBook;
        warmupBook.setVerbose(false);

        for (int i = 1; i <= numOrders; ++i)
        {
            warmupBook.addOrder({
                i,
                Side::Sell,
                OrderType::Limit,
                100,
                10,
                static_cast<uint64_t>(i)
            });
        }

        for (int i = 1; i <= numOrders; ++i)
        {
            warmupBook.addOrder({
                numOrders + i,
                Side::Buy,
                OrderType::Limit,
                100,
                10,
                static_cast<uint64_t>(numOrders + i)
            });
        }
    }

    constexpr int iterations = 5;

    for (int run = 0; run < iterations; ++run)
    {
        OrderBook book;
        book.setVerbose(false);

        // Populate book with SELL orders
        for (int i = 1; i <= numOrders; ++i)
        {
            book.addOrder({
                i,
                Side::Sell,
                OrderType::Limit,
                100,
                10,
                static_cast<uint64_t>(i)
            });
        }

        runner.reset();

        runner.start();

        // Matching benchmark
        for (int i = 1; i <= numOrders; ++i)
        {
            book.addOrder({
                numOrders + i,
                Side::Buy,
                OrderType::Limit,
                100,
                10,
                static_cast<uint64_t>(numOrders + i)
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
        benchmarkMatching(workload);
    }

    return 0;
}