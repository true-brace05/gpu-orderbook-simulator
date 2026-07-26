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
    std::cout << "====================================================================================================================================\n";
    std::cout << "                                  GPU Price Level Builder Performance Benchmark                                     \n";
    std::cout << "====================================================================================================================================\n\n";

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
                  << std::setw(12) << "Add Events"
                  << std::setw(14) << "Unique Levels"
                  << std::setw(14) << "Agg Ratio"
                  << std::setw(14) << "Avg Ord/Lvl"
                  << std::setw(16) << "Kernel Time(ms)"
                  << std::setw(16) << "E2E Time(ms)"
                  << std::setw(16) << "M Events/sec"
                  << std::setw(18) << "Effective GB/s"
                  << std::setw(12) << "Verified"
                  << '\n';
        std::cout << "------------------------------------------------------------------------------------------------------------------------------------\n";

        std::mt19937 rng(424242);
        std::uniform_int_distribution<uint64_t> timeDist(1000000, 9000000);
        std::uniform_int_distribution<int> sideDist(0, 1);
        std::uniform_int_distribution<int> qtyDist(10, 500);

        for (std::size_t N : batchSizes)
        {
            // Number of unique price ticks scales with batch size to simulate real LOB density
            const std::size_t numUniquePrices = std::max<std::size_t>(5, N / 50);
            std::uniform_int_distribution<int> priceIdxDist(0, static_cast<int>(numUniquePrices - 1));

            std::vector<double> fixedPrices(numUniquePrices);
            for (std::size_t i = 0; i < numUniquePrices; ++i)
            {
                fixedPrices[i] = 100.0 + static_cast<double>(i) * 0.50;
            }

            std::vector<Event> h_events(N);
            for (std::size_t i = 0; i < N; ++i)
            {
                h_events[i].timestamp = timeDist(rng);
                h_events[i].type = EventType::Add; // 100% Add events
                h_events[i].orderId = static_cast<int>(i + 1);
                h_events[i].order.id = h_events[i].orderId;
                h_events[i].order.side = static_cast<Side>(sideDist(rng));
                h_events[i].order.type = OrderType::Limit;
                h_events[i].order.price = fixedPrices[priceIdxDist(rng)];
                h_events[i].order.quantity = qtyDist(rng);
                h_events[i].order.displayQuantity = h_events[i].order.quantity;
                h_events[i].order.reserveQuantity = 0;
            }

            const std::size_t bytesTransferred = N * (sizeof(double) + sizeof(int) + sizeof(uint8_t)) +
                                                 N * (sizeof(PriceLevel) + sizeof(int));
            const double dataSizeGB = static_cast<double>(bytesTransferred) / (1024.0 * 1024.0 * 1024.0);

            ReplayBuffer replayBuf(N);
            DecodedEventBuffer decodedBuf(N);
            ClassifiedEventBuffer classifiedBuf(N);
            PriceLevelBuffer levelBuf(N);

            replayBuf.uploadBatch(h_events.data(), N, context.getStream());
            decodeEvents(replayBuf, decodedBuf, context);
            classifyEvents(decodedBuf, classifiedBuf, context);

            // Warmup execution
            buildPriceLevels(decodedBuf, classifiedBuf, levelBuf, context, tickSize);

            std::size_t uniqueLevels = levelBuf.size();
            double aggRatio = (uniqueLevels > 0) ? (static_cast<double>(N) / static_cast<double>(uniqueLevels)) : 0.0;
            double avgOrdersPerLevel = aggRatio;

            // --- End-to-End Timing (Host Upload + Decode + Classify + Level Build) ---
            constexpr int iterations = 10;
            auto t0_e2e = std::chrono::high_resolution_clock::now();

            for (int it = 0; it < iterations; ++it)
            {
                replayBuf.uploadBatch(h_events.data(), N, context.getStream());
                decodeEventsAsync(replayBuf, decodedBuf, context.getStream());
                classifyEventsAsync(decodedBuf, classifiedBuf, context.getStream());
                buildPriceLevelsAsync(decodedBuf, classifiedBuf, levelBuf, context.getStream(), tickSize);
            }
            context.synchronize();

            auto t1_e2e = std::chrono::high_resolution_clock::now();
            double totalE2EMs = std::chrono::duration<double, std::milli>(t1_e2e - t0_e2e).count();
            double avgE2EMs = totalE2EMs / iterations;

            // --- Kernel Only Timing (CUDA Events for Price Level Builder) ---
            context.startTimer();
            for (int it = 0; it < iterations; ++it)
            {
                buildPriceLevelsAsync(decodedBuf, classifiedBuf, levelBuf, context.getStream(), tickSize);
            }
            context.stopTimer();
            double totalKernelMs = context.elapsedMilliseconds();
            double avgKernelMs = totalKernelMs / iterations;

            double mEventsPerSec = (avgKernelMs > 0.0) ? ((static_cast<double>(N) / 1e6) / (avgKernelMs / 1000.0)) : 0.0;
            double effectiveGBps = (avgKernelMs > 0.0) ? (dataSizeGB / (avgKernelMs / 1000.0)) : 0.0;

            bool isCorrect = (uniqueLevels > 0 && uniqueLevels <= N);

            std::cout << std::left
                      << std::setw(12) << N
                      << std::setw(14) << uniqueLevels
                      << std::setw(14) << std::fixed << std::setprecision(2) << aggRatio
                      << std::setw(14) << std::fixed << std::setprecision(2) << avgOrdersPerLevel
                      << std::setw(16) << std::fixed << std::setprecision(3) << avgKernelMs
                      << std::setw(16) << std::fixed << std::setprecision(3) << avgE2EMs
                      << std::setw(16) << std::fixed << std::setprecision(2) << mEventsPerSec
                      << std::setw(18) << std::fixed << std::setprecision(2) << effectiveGBps
                      << std::setw(12) << (isCorrect ? "✓ PASS" : "✗ FAIL")
                      << '\n';
        }

        std::cout << "====================================================================================================================================\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal Error: PriceLevelBuilder benchmark failed: " << ex.what() << '\n';
        return 1;
    }
}
