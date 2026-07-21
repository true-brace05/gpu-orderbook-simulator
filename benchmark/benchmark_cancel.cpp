#include "OrderBook.h"
#include "workloads.h"
#include "performance/BenchmarkRunner.h"
#include "performance/ReportPrinter.h"

void benchmarkCancellation(int numOrders)
{
    BenchmarkRunner runner("Cancellation Benchmark");

    // ---------------------------------------
    // Warm-up (NOT measured)
    // ---------------------------------------
    {
        OrderBook warmupBook;

        for (int i = 1; i <= numOrders; ++i)
        {
            warmupBook.addOrder({
                i,
                Side::Buy,
                OrderType::Limit,
                100,
                10,
                static_cast<uint64_t>(i)
            });
        }

        for (int i = 1; i <= numOrders; ++i)
        {
            warmupBook.cancelOrder(i);
        }
    }

    constexpr int iterations = 5;

    for (int run = 0; run < iterations; ++run)
    {
        OrderBook book;

        // Populate the order book
        for (int i = 1; i <= numOrders; ++i)
        {
            book.addOrder({
                i,
                Side::Buy,
                OrderType::Limit,
                100,
                10,
                static_cast<uint64_t>(i)
            });
        }

        runner.reset();

        runner.start();

        for (int i = 1; i <= numOrders; ++i)
        {
            book.cancelOrder(i);
        }

        runner.stop(numOrders);
    }

    ReportPrinter::print(runner.result());
}

int main()
{
    for (int workload : workloads)
    {
        benchmarkCancellation(workload);
    }

    return 0;
}