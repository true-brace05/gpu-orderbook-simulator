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
    std::cout << "=====================================================================================================================\n";
    std::cout << "                          GPU Event Classification Performance Benchmark                             \n";
    std::cout << "=====================================================================================================================\n\n";

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

        std::cout << std::left
                  << std::setw(12) << "Batch Size"
                  << std::setw(14) << "Data Size(MB)"
                  << std::setw(16) << "Kernel Time(ms)"
                  << std::setw(16) << "E2E Time(ms)"
                  << std::setw(16) << "M Events/sec"
                  << std::setw(18) << "Effective GB/s"
                  << std::setw(12) << "Verified"
                  << '\n';
        std::cout << "---------------------------------------------------------------------------------------------------------------------\n";

        std::mt19937 rng(12345);
        // Skewed weights: 60% Add, 25% Cancel, 8% Modify, 3% Delete, 2% ExVis, 1% ExHid, 1% Halt
        std::vector<double> categoryWeights = {60.0, 25.0, 8.0, 3.0, 2.0, 1.0, 1.0};
        std::discrete_distribution<int> dist(categoryWeights.begin(), categoryWeights.end());
        std::uniform_int_distribution<uint64_t> timeDist(1000000, 9000000);

        for (std::size_t N : batchSizes)
        {
            std::vector<Event> h_events(N);
            std::array<std::size_t, 7> cpuCounts = {0};

            for (std::size_t i = 0; i < N; ++i)
            {
                int cat = dist(rng);
                h_events[i].timestamp = timeDist(rng);
                h_events[i].type = static_cast<EventType>(cat);
                h_events[i].orderId = static_cast<int>(i + 1);
                h_events[i].order.id = h_events[i].orderId;
                h_events[i].order.side = (i % 2 == 0) ? Side::Buy : Side::Sell;
                h_events[i].order.type = OrderType::Limit;
                h_events[i].order.price = 100.0 + (i % 50);
                h_events[i].order.quantity = 10;
                h_events[i].order.displayQuantity = 10;
                h_events[i].order.reserveQuantity = 0;

                cpuCounts[cat]++;
            }

            const std::size_t bytesTransferred = N * (sizeof(uint8_t) + sizeof(int));
            const double dataSizeMB = static_cast<double>(bytesTransferred) / (1024.0 * 1024.0);
            const double dataSizeGB = static_cast<double>(bytesTransferred) / (1024.0 * 1024.0 * 1024.0);

            ReplayBuffer replayBuf(N);
            DecodedEventBuffer decodedBuf(N);
            ClassifiedEventBuffer classifiedBuf(N);

            replayBuf.uploadBatch(h_events.data(), N, context.getStream());
            decodeEvents(replayBuf, decodedBuf, context);

            // Warmup execution
            classifyEvents(decodedBuf, classifiedBuf, context);

            // --- End-to-End Timing (Host Upload + Decode + Classify + Counter Sync) ---
            constexpr int iterations = 10;
            auto t0_e2e = std::chrono::high_resolution_clock::now();

            for (int it = 0; it < iterations; ++it)
            {
                replayBuf.uploadBatch(h_events.data(), N, context.getStream());
                decodeEventsAsync(replayBuf, decodedBuf, context.getStream());
                classifyEventsAsync(decodedBuf, classifiedBuf, context.getStream());
            }
            context.synchronize();

            auto t1_e2e = std::chrono::high_resolution_clock::now();
            double totalE2EMs = std::chrono::duration<double, std::milli>(t1_e2e - t0_e2e).count();
            double avgE2EMs = totalE2EMs / iterations;

            // --- Kernel Only Timing (CUDA Events for Classification Kernel) ---
            context.startTimer();
            for (int it = 0; it < iterations; ++it)
            {
                classifyEventsAsync(decodedBuf, classifiedBuf, context.getStream());
            }
            context.stopTimer();
            double totalKernelMs = context.elapsedMilliseconds();
            double avgKernelMs = totalKernelMs / iterations;

            double mEventsPerSec = (avgKernelMs > 0.0) ? ((static_cast<double>(N) / 1e6) / (avgKernelMs / 1000.0)) : 0.0;
            double effectiveGBps = (avgKernelMs > 0.0) ? (dataSizeGB / (avgKernelMs / 1000.0)) : 0.0;

            // Verification
            bool isCorrect = (classifiedBuf.size() == N);
            for (int c = 0; c < 7; ++c)
            {
                if (classifiedBuf.getCount(static_cast<EventType>(c)) != cpuCounts[c])
                {
                    isCorrect = false;
                    break;
                }
            }

            std::cout << std::left
                      << std::setw(12) << N
                      << std::setw(14) << std::fixed << std::setprecision(2) << dataSizeMB
                      << std::setw(16) << std::fixed << std::setprecision(3) << avgKernelMs
                      << std::setw(16) << std::fixed << std::setprecision(3) << avgE2EMs
                      << std::setw(16) << std::fixed << std::setprecision(2) << mEventsPerSec
                      << std::setw(18) << std::fixed << std::setprecision(2) << effectiveGBps
                      << std::setw(12) << (isCorrect ? "✓ PASS" : "✗ FAIL")
                      << '\n';

            // Print per-category breakdown
            std::cout << "   └─ Per-Category Counts: "
                      << "Add: " << classifiedBuf.getCount(EventType::Add)
                      << " | Cancel: " << classifiedBuf.getCount(EventType::Cancel)
                      << " | Modify: " << classifiedBuf.getCount(EventType::Modify)
                      << " | Delete: " << classifiedBuf.getCount(EventType::Delete)
                      << " | ExVis: " << classifiedBuf.getCount(EventType::ExecuteVisible)
                      << " | ExHid: " << classifiedBuf.getCount(EventType::ExecuteHidden)
                      << " | Halt: " << classifiedBuf.getCount(EventType::TradingHalt)
                      << "\n\n";
        }

        std::cout << "=====================================================================================================================\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal Error: EventClassifier benchmark failed: " << ex.what() << '\n';
        return 1;
    }
}
