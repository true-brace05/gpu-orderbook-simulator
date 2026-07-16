
#include "BenchmarkUtils.h"
#include "OrderBook.h"
#include "workloads.h"

void benchmarkMatching(int numOrders)
{
    OrderBook book;

    book.setVerbose(false);

    // Fill book with SELL orders

    for (int i = 1; i <= numOrders; ++i)
    {
        book.addOrder(
        {
            i,
            Side::Sell,
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
        book.addOrder(
        {
            numOrders + i,
            Side::Buy,
             OrderType::Limit,
            100,
            10,
            static_cast<uint64_t>(numOrders + i)
        });
    }

    auto end =
        std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<
            std::chrono::microseconds>(
                end - start);

    printBenchmarkResult(
        "Matching Benchmark",
        numOrders,
        duration);
}

int main()
{
    for(int workload : workloads)
    {
        benchmarkMatching(workload);
    }
}