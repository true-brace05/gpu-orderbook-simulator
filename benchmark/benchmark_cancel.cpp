#include "BenchmarkUtils.h"
#include "OrderBook.h"
#include "workloads.h"


void benchmarkCancellation(int numOrders)
{
    OrderBook book;

    for (int i = 1; i <= numOrders; ++i)
    {
        book.addOrder(
        {
            i,
            Side::Buy,
            OrderType::Limit,
            100,
            10,
            static_cast<uint64_t>(i)
        });
    }

    auto start =
        std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= numOrders; ++i)
    {
        book.cancelOrder(i);
    }

    auto end =
        std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<
            std::chrono::microseconds>(
                end - start);

    printBenchmarkResult(
        "Cancellation Benchmark",
        numOrders,
        duration);
}

int main()
{
    for(int workload : workloads)
    {
        benchmarkCancellation(workload);
    }
}