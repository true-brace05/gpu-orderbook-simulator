#include "BenchmarkUtils.h"
#include "OrderBook.h"
#include "workloads.h"

void benchmarkInsertion(int numOrders)
{
    OrderBook book;

    auto start =
        std::chrono::high_resolution_clock::now();

    for (int i = 1; i <= numOrders; ++i)
    {
        book.addOrder(
        {
            i,
            Side::Buy,
            100,
            10,
            i
        });
    }

    auto end =
        std::chrono::high_resolution_clock::now();

    auto duration =
        std::chrono::duration_cast<
            std::chrono::microseconds>(
                end - start);

    printBenchmarkResult(
        "Insertion Benchmark",
        numOrders,
        duration);
}

int main()
{
    for(int workload : workloads)
    {
        benchmarkInsertion(workload);
    }
}