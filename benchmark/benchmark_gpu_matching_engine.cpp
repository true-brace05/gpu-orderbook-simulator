#include "cuda/GPUMatchingEngine.h"
#include "cuda/TradeBuffer.h"
#include "cuda/PriceLevelBuilder.h"
#include "cuda/PriceLevelBuffer.h"
#include "cuda/EventClassifier.h"
#include "cuda/EventDecoder.h"
#include "cuda/ReplayBuffer.h"
#include "cuda/DecodedEventBuffer.h"
#include "cuda/ClassifiedEventBuffer.h"
#include "cuda/CUDAContext.h"
#include "replay/Event.h"
#include "Types.h"

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <stdexcept>

int main()
{
    std::cout << "=========================================================================================================================================\n";
    std::cout << "                                   GPU Matching Engine Performance Benchmark                                             \n";
    std::cout << "=========================================================================================================================================\n\n";

    try
    {
        CUDAContext context;

        const std::vector<std::size_t> batchSizes = {
            1000,         // 1K events
            10000,        // 10K events
            100000,       // 100K events
            1000000,      // 1M events
            10000000      // 10M events
        };

        constexpr double tickSize = 0.01;

        std::cout << std::left
                  << std::setw(12) << "Orders"
                  << std::setw(12) << "Trades"
                  << std::setw(12) << "FullFill"
                  << std::setw(12) << "PartFill"
                  << std::setw(12) << "Rested"
                  << std::setw(16) << "Kernel Time(ms)"
                  << std::setw(16) << "E2E Time(ms)"
                  << std::setw(16) << "M Orders/sec"
                  << std::setw(18) << "Effective GB/s"
                  << std::setw(12) << "Verified"
                  << '\n';
        std::cout << "-----------------------------------------------------------------------------------------------------------------------------------------\n";

        std::mt19937 rng(1337);
        std::uniform_int_distribution<uint64_t> timeDist(1000000, 9000000);
        std::uniform_int_distribution<int> qtyDist(10, 500);

        for (std::size_t N : batchSizes)
        {
            // Populate resting book with Sells at prices $100.00 to $110.00
            const std::size_t bookSize = N / 2;
            std::vector<Event> restingEvents(bookSize);
            for (std::size_t i = 0; i < bookSize; ++i)
            {
                restingEvents[i].timestamp = timeDist(rng);
                restingEvents[i].type = EventType::Add;
                restingEvents[i].orderId = static_cast<int>(i + 1);
                restingEvents[i].order.id = restingEvents[i].orderId;
                restingEvents[i].order.side = Side::Sell;
                restingEvents[i].order.type = OrderType::Limit;
                restingEvents[i].order.price = 100.0 + static_cast<double>(i % 100) * 0.10;
                restingEvents[i].order.quantity = qtyDist(rng);
                restingEvents[i].order.displayQuantity = restingEvents[i].order.quantity;
                restingEvents[i].order.reserveQuantity = 0;
            }

            // Incoming Buys at prices $99.50 to $105.00
            std::vector<Event> incEvents(N);
            for (std::size_t i = 0; i < N; ++i)
            {
                incEvents[i].timestamp = timeDist(rng);
                incEvents[i].type = EventType::Add;
                incEvents[i].orderId = static_cast<int>(bookSize + i + 1);
                incEvents[i].order.id = incEvents[i].orderId;
                incEvents[i].order.side = Side::Buy;
                incEvents[i].order.type = OrderType::Limit;
                incEvents[i].order.price = 99.50 + static_cast<double>(i % 60) * 0.10;
                incEvents[i].order.quantity = qtyDist(rng);
                incEvents[i].order.displayQuantity = incEvents[i].order.quantity;
                incEvents[i].order.reserveQuantity = 0;
            }

            ReplayBuffer restReplay(bookSize);
            DecodedEventBuffer restDecoded(bookSize);
            ClassifiedEventBuffer restClassified(bookSize);
            PriceLevelBuffer restingBook(bookSize);

            restReplay.uploadBatch(restingEvents.data(), bookSize, context.getStream());
            decodeEvents(restReplay, restDecoded, context);
            classifyEvents(restDecoded, restClassified, context);
            buildPriceLevels(restDecoded, restClassified, restingBook, context, tickSize);

            ReplayBuffer incReplay(N);
            DecodedEventBuffer incDecoded(N);
            ClassifiedEventBuffer incClassified(N);

            incReplay.uploadBatch(incEvents.data(), N, context.getStream());
            decodeEvents(incReplay, incDecoded, context);
            classifyEvents(incDecoded, incClassified, context);

            TradeBuffer tradeBuf(N * 2);
            MatchingStatistics stats;

            // Warmup execution
            matchAddOrders(incDecoded, incClassified, restDecoded, restingBook, tradeBuf, stats, context, tickSize);

            const std::size_t bytesTransferred = N * (sizeof(double) + sizeof(int) + sizeof(uint8_t)) +
                                                 stats.tradesGenerated * sizeof(TradeRecord);
            const double dataSizeGB = static_cast<double>(bytesTransferred) / (1024.0 * 1024.0 * 1024.0);

            // --- End-to-End Timing ---
            constexpr int iterations = 10;
            auto t0_e2e = std::chrono::high_resolution_clock::now();

            for (int it = 0; it < iterations; ++it)
            {
                incReplay.uploadBatch(incEvents.data(), N, context.getStream());
                decodeEventsAsync(incReplay, incDecoded, context.getStream());
                classifyEventsAsync(incDecoded, incClassified, context.getStream());
                matchAddOrdersAsync(incDecoded, incClassified, restDecoded, restingBook, tradeBuf, stats, context.getStream(), tickSize);
            }
            context.synchronize();

            auto t1_e2e = std::chrono::high_resolution_clock::now();
            double totalE2EMs = std::chrono::duration<double, std::milli>(t1_e2e - t0_e2e).count();
            double avgE2EMs = totalE2EMs / iterations;

            // --- Kernel Only Timing ---
            context.startTimer();
            for (int it = 0; it < iterations; ++it)
            {
                matchAddOrdersAsync(incDecoded, incClassified, restDecoded, restingBook, tradeBuf, stats, context.getStream(), tickSize);
            }
            context.stopTimer();
            double totalKernelMs = context.elapsedMilliseconds();
            double avgKernelMs = totalKernelMs / iterations;

            double mOrdersPerSec = (avgKernelMs > 0.0) ? ((static_cast<double>(N) / 1e6) / (avgKernelMs / 1000.0)) : 0.0;
            double effectiveGBps = (avgKernelMs > 0.0) ? (dataSizeGB / (avgKernelMs / 1000.0)) : 0.0;

            bool isCorrect = (stats.fullyMatchedOrders + stats.partiallyMatchedOrders + stats.restedOrders == N);

            std::cout << std::left
                      << std::setw(12) << N
                      << std::setw(12) << stats.tradesGenerated
                      << std::setw(12) << stats.fullyMatchedOrders
                      << std::setw(12) << stats.partiallyMatchedOrders
                      << std::setw(12) << stats.restedOrders
                      << std::setw(16) << std::fixed << std::setprecision(3) << avgKernelMs
                      << std::setw(16) << std::fixed << std::setprecision(3) << avgE2EMs
                      << std::setw(16) << std::fixed << std::setprecision(2) << mOrdersPerSec
                      << std::setw(18) << std::fixed << std::setprecision(2) << effectiveGBps
                      << std::setw(12) << (isCorrect ? "✓ PASS" : "✗ FAIL")
                      << '\n';
        }

        std::cout << "=========================================================================================================================================\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal Error: GPUMatchingEngine benchmark failed: " << ex.what() << '\n';
        return 1;
    }
}
