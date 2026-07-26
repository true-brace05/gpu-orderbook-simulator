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
#include <string>
#include <cstring>

#if defined(__CUDACC__) || defined(__CUDA_RUNTIME_H__)
#include <cuda_runtime.h>
#endif

// -----------------------------------------------------------------------------
// Helper: Print System & GPU Info
// -----------------------------------------------------------------------------
void printSystemInfo()
{
    std::cout << "=========================================================================================================================================\n";
    std::cout << "                                   GPU Matching Engine Performance Benchmark                                             \n";
    std::cout << "=========================================================================================================================================\n";

#if defined(__CUDACC__) || defined(__CUDA_RUNTIME_H__)
    int deviceId = 0;
    cudaError_t err = cudaGetDevice(&deviceId);
    if (err == cudaSuccess)
    {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, deviceId);
        int runtimeVersion = 0;
        cudaRuntimeGetVersion(&runtimeVersion);

        std::cout << "GPU Device         : " << prop.name << '\n';
        std::cout << "CUDA Runtime       : " << (runtimeVersion / 1000) << '.' << ((runtimeVersion % 100) / 10) << '\n';
        std::cout << "Compute Capability : " << prop.major << '.' << prop.minor << '\n';
        std::cout << "Global Memory      : " << (prop.totalGlobalMem / (1024 * 1024)) << " MB\n";
    }
    else
    {
        std::cout << "GPU Device         : Host Emulation / CPU Mode\n";
    }
#else
    std::cout << "GPU Device         : Host Emulation / CPU Mode\n";
#endif

    std::cout << "=========================================================================================================================================\n\n";
}

// -----------------------------------------------------------------------------
// Benchmark Scenario Enum
// -----------------------------------------------------------------------------
enum class Scenario
{
    NoMatch,    // Scenario A: Non-crossing prices (0 trades)
    FullMatch,  // Scenario B: Exact price crossing (100% full fills)
    MixedMarket // Scenario C: 70% matching, 30% non-matching (realistic exchange workload)
};

std::string getScenarioName(Scenario sc)
{
    switch (sc)
    {
        case Scenario::NoMatch:    return "Scenario A — No Match (Search Overhead)";
        case Scenario::FullMatch:  return "Scenario B — Full Match (Throughput Limit)";
        case Scenario::MixedMarket: return "Scenario C — Mixed Market (70% Match / 30% No-Match)";
    }
    return "Unknown Scenario";
}

// -----------------------------------------------------------------------------
// Data Generator per Scenario
// -----------------------------------------------------------------------------
void generateMarketData(
    Scenario scenario,
    std::size_t N,
    std::vector<Event>& outRestingEvents,
    std::vector<Event>& outIncEvents)
{
    const std::size_t bookSize = N / 2;
    outRestingEvents.resize(bookSize);
    outIncEvents.resize(N);

    std::mt19937 rng(1337);
    std::uniform_int_distribution<uint64_t> timeDist(1000000, 9000000);
    std::uniform_int_distribution<int> qtyDist(10, 200);

    if (scenario == Scenario::NoMatch)
    {
        // Resting Sells at $105.00 - $115.00
        for (std::size_t i = 0; i < bookSize; ++i)
        {
            outRestingEvents[i].timestamp = timeDist(rng);
            outRestingEvents[i].type = EventType::Add;
            outRestingEvents[i].orderId = static_cast<int>(i + 1);
            outRestingEvents[i].order.id = outRestingEvents[i].orderId;
            outRestingEvents[i].order.side = Side::Sell;
            outRestingEvents[i].order.type = OrderType::Limit;
            outRestingEvents[i].order.price = 105.00 + static_cast<double>(i % 50) * 0.20;
            outRestingEvents[i].order.quantity = qtyDist(rng);
            outRestingEvents[i].order.displayQuantity = outRestingEvents[i].order.quantity;
            outRestingEvents[i].order.reserveQuantity = 0;
        }

        // Incoming Buys at $90.00 - $100.00 (No crossing)
        for (std::size_t i = 0; i < N; ++i)
        {
            outIncEvents[i].timestamp = timeDist(rng);
            outIncEvents[i].type = EventType::Add;
            outIncEvents[i].orderId = static_cast<int>(bookSize + i + 1);
            outIncEvents[i].order.id = outIncEvents[i].orderId;
            outIncEvents[i].order.side = Side::Buy;
            outIncEvents[i].order.type = OrderType::Limit;
            outIncEvents[i].order.price = 90.00 + static_cast<double>(i % 50) * 0.20;
            outIncEvents[i].order.quantity = qtyDist(rng);
            outIncEvents[i].order.displayQuantity = outIncEvents[i].order.quantity;
            outIncEvents[i].order.reserveQuantity = 0;
        }
    }
    else if (scenario == Scenario::FullMatch)
    {
        // Resting Sells at $100.00 (Qty 100 each)
        for (std::size_t i = 0; i < bookSize; ++i)
        {
            outRestingEvents[i].timestamp = timeDist(rng);
            outRestingEvents[i].type = EventType::Add;
            outRestingEvents[i].orderId = static_cast<int>(i + 1);
            outRestingEvents[i].order.id = outRestingEvents[i].orderId;
            outRestingEvents[i].order.side = Side::Sell;
            outRestingEvents[i].order.type = OrderType::Limit;
            outRestingEvents[i].order.price = 100.00 + static_cast<double>(i % 10) * 0.10;
            outRestingEvents[i].order.quantity = 100;
            outRestingEvents[i].order.displayQuantity = 100;
            outRestingEvents[i].order.reserveQuantity = 0;
        }

        // Incoming Buys at $100.00 (Qty 100 each -> 100% full fills)
        for (std::size_t i = 0; i < N; ++i)
        {
            outIncEvents[i].timestamp = timeDist(rng);
            outIncEvents[i].type = EventType::Add;
            outIncEvents[i].orderId = static_cast<int>(bookSize + i + 1);
            outIncEvents[i].order.id = outIncEvents[i].orderId;
            outIncEvents[i].order.side = Side::Buy;
            outIncEvents[i].order.type = OrderType::Limit;
            outIncEvents[i].order.price = 100.00 + static_cast<double>(i % 10) * 0.10;
            outIncEvents[i].order.quantity = 100;
            outIncEvents[i].order.displayQuantity = 100;
            outIncEvents[i].order.reserveQuantity = 0;
        }
    }
    else // MixedMarket
    {
        // Resting Sells at $100.00 - $110.00
        for (std::size_t i = 0; i < bookSize; ++i)
        {
            outRestingEvents[i].timestamp = timeDist(rng);
            outRestingEvents[i].type = EventType::Add;
            outRestingEvents[i].orderId = static_cast<int>(i + 1);
            outRestingEvents[i].order.id = outRestingEvents[i].orderId;
            outRestingEvents[i].order.side = Side::Sell;
            outRestingEvents[i].order.type = OrderType::Limit;
            outRestingEvents[i].order.price = 100.00 + static_cast<double>(i % 50) * 0.20;
            outRestingEvents[i].order.quantity = qtyDist(rng);
            outRestingEvents[i].order.displayQuantity = outRestingEvents[i].order.quantity;
            outRestingEvents[i].order.reserveQuantity = 0;
        }

        // Incoming: 70% crossing Buys ($100.00 - $108.00), 30% non-crossing Buys ($90.00 - $98.00)
        std::uniform_int_distribution<int> pctDist(1, 100);
        for (std::size_t i = 0; i < N; ++i)
        {
            outIncEvents[i].timestamp = timeDist(rng);
            outIncEvents[i].type = EventType::Add;
            outIncEvents[i].orderId = static_cast<int>(bookSize + i + 1);
            outIncEvents[i].order.id = outIncEvents[i].orderId;
            outIncEvents[i].order.side = Side::Buy;
            outIncEvents[i].order.type = OrderType::Limit;

            if (pctDist(rng) <= 70)
            {
                // 70% Match: Price $100.00 - $108.00
                outIncEvents[i].order.price = 100.00 + static_cast<double>(i % 40) * 0.20;
            }
            else
            {
                // 30% No-Match: Price $90.00 - $98.00
                outIncEvents[i].order.price = 90.00 + static_cast<double>(i % 40) * 0.20;
            }

            outIncEvents[i].order.quantity = qtyDist(rng);
            outIncEvents[i].order.displayQuantity = outIncEvents[i].order.quantity;
            outIncEvents[i].order.reserveQuantity = 0;
        }
    }
}

// -----------------------------------------------------------------------------
// Run Benchmark Function
// -----------------------------------------------------------------------------
void runBenchmark(Scenario scenario, const std::vector<std::size_t>& batchSizes)
{
    constexpr double tickSize = 0.01;
    constexpr int iterations = 5;

    std::cout << "-----------------------------------------------------------------------------------------------------------------------------------------\n";
    std::cout << "  " << getScenarioName(scenario) << '\n';
    std::cout << "-----------------------------------------------------------------------------------------------------------------------------------------\n";

    std::cout << std::left
              << std::setw(10) << "Orders"
              << std::setw(10) << "Trades"
              << std::setw(10) << "Levels"
              << std::setw(10) << "AvgOrd/Lvl"
              << std::setw(11) << "Upload(ms)"
              << std::setw(11) << "Decode(ms)"
              << std::setw(11) << "Class(ms)"
              << std::setw(11) << "PBuild(ms)"
              << std::setw(11) << "Match(ms)"
              << std::setw(11) << "Dnload(ms)"
              << std::setw(11) << "Verify(ms)"
              << std::setw(11) << "Total(ms)"
              << std::setw(13) << "M Orders/s"
              << std::setw(13) << "M Trades/s"
              << std::setw(14) << "EffectiveGB/s"
              << '\n';
    std::cout << "-----------------------------------------------------------------------------------------------------------------------------------------\n";

    CUDAContext context;

    for (std::size_t N : batchSizes)
    {
        const std::size_t bookSize = N / 2;

        std::vector<Event> restingEvents;
        std::vector<Event> incEvents;
        generateMarketData(scenario, N, restingEvents, incEvents);

        double totalUploadMs = 0.0;
        double totalDecodeMs = 0.0;
        double totalClassifyMs = 0.0;
        double totalPBuildMs = 0.0;
        double totalMatchMs = 0.0;
        double totalDownloadMs = 0.0;
        double totalVerifyMs = 0.0;
        double totalE2EMs = 0.0;

        std::size_t numPriceLevels = 0;
        double avgOrdersPerLevel = 0.0;
        MatchingStatistics finalStats;

        for (int it = 0; it < iterations; ++it)
        {
            auto t_start_e2e = std::chrono::high_resolution_clock::now();

            // 1. Upload H->D
            auto t0 = std::chrono::high_resolution_clock::now();
            ReplayBuffer restReplay(bookSize);
            ReplayBuffer incReplay(N);
            restReplay.uploadBatch(restingEvents.data(), bookSize, context.getStream());
            incReplay.uploadBatch(incEvents.data(), N, context.getStream());
            context.synchronize();
            auto t1 = std::chrono::high_resolution_clock::now();
            totalUploadMs += std::chrono::duration<double, std::milli>(t1 - t0).count();

            // 2. Event Decode
            t0 = std::chrono::high_resolution_clock::now();
            DecodedEventBuffer restDecoded(bookSize);
            DecodedEventBuffer incDecoded(N);
            decodeEventsAsync(restReplay, restDecoded, context.getStream());
            decodeEventsAsync(incReplay, incDecoded, context.getStream());
            context.synchronize();
            t1 = std::chrono::high_resolution_clock::now();
            totalDecodeMs += std::chrono::duration<double, std::milli>(t1 - t0).count();

            // 3. Event Classification
            t0 = std::chrono::high_resolution_clock::now();
            ClassifiedEventBuffer restClassified(bookSize);
            ClassifiedEventBuffer incClassified(N);
            classifyEventsAsync(restDecoded, restClassified, context.getStream());
            classifyEventsAsync(incDecoded, incClassified, context.getStream());
            context.synchronize();
            t1 = std::chrono::high_resolution_clock::now();
            totalClassifyMs += std::chrono::duration<double, std::milli>(t1 - t0).count();

            // 4. Price Level Builder (Rebuilt fresh every iteration!)
            t0 = std::chrono::high_resolution_clock::now();
            PriceLevelBuffer restingBook(bookSize);
            buildPriceLevelsAsync(restDecoded, restClassified, restingBook, context.getStream(), tickSize);
            context.synchronize();
            t1 = std::chrono::high_resolution_clock::now();
            totalPBuildMs += std::chrono::duration<double, std::milli>(t1 - t0).count();

            numPriceLevels = restingBook.size();
            avgOrdersPerLevel = (numPriceLevels > 0) ? (static_cast<double>(bookSize) / numPriceLevels) : 0.0;

            // 5. GPU Matching Engine Kernel
            t0 = std::chrono::high_resolution_clock::now();
            TradeBuffer tradeBuf(N * 2);
            MatchingStatistics stats;
            matchAddOrdersAsync(incDecoded, incClassified, restDecoded, restingBook, tradeBuf, stats, context.getStream(), tickSize);
            context.synchronize();
            t1 = std::chrono::high_resolution_clock::now();
            totalMatchMs += std::chrono::duration<double, std::milli>(t1 - t0).count();

            // 6. Download D->H
            t0 = std::chrono::high_resolution_clock::now();
            std::vector<TradeRecord> h_trades(tradeBuf.size());
            if (tradeBuf.size() > 0)
            {
                tradeBuf.getTradesBuffer().copyToHostAsync(h_trades.data(), tradeBuf.size(), context.getStream());
                context.synchronize();
            }
            t1 = std::chrono::high_resolution_clock::now();
            totalDownloadMs += std::chrono::duration<double, std::milli>(t1 - t0).count();

            // 7. Verification
            t0 = std::chrono::high_resolution_clock::now();
            bool isCorrect = (stats.fullyMatchedOrders + stats.partiallyMatchedOrders + stats.restedOrders == N);
            t1 = std::chrono::high_resolution_clock::now();
            totalVerifyMs += std::chrono::duration<double, std::milli>(t1 - t0).count();

            if (!isCorrect)
            {
                std::cerr << "Warning: Verification check failed at iteration " << it << '\n';
            }

            auto t_end_e2e = std::chrono::high_resolution_clock::now();
            totalE2EMs += std::chrono::duration<double, std::milli>(t_end_e2e - t_start_e2e).count();

            finalStats = stats;
        }

        // Averages across iterations
        double avgUploadMs   = totalUploadMs / iterations;
        double avgDecodeMs   = totalDecodeMs / iterations;
        double avgClassMs    = totalClassifyMs / iterations;
        double avgPBuildMs   = totalPBuildMs / iterations;
        double avgMatchMs    = totalMatchMs / iterations;
        double avgDnloadMs   = totalDownloadMs / iterations;
        double avgVerifyMs   = totalVerifyMs / iterations;
        double avgTotalMs    = totalE2EMs / iterations;

        double mOrdersPerSec = (avgMatchMs > 0.0) ? ((static_cast<double>(N) / 1e6) / (avgMatchMs / 1000.0)) : 0.0;
        double mTradesPerSec = (avgMatchMs > 0.0) ? ((static_cast<double>(finalStats.tradesGenerated) / 1e6) / (avgMatchMs / 1000.0)) : 0.0;

        const std::size_t bytesTransferred = N * (sizeof(double) + sizeof(int) + sizeof(uint8_t)) +
                                             finalStats.tradesGenerated * sizeof(TradeRecord);
        const double dataSizeGB = static_cast<double>(bytesTransferred) / (1024.0 * 1024.0 * 1024.0);
        double effectiveGBps = (avgMatchMs > 0.0) ? (dataSizeGB / (avgMatchMs / 1000.0)) : 0.0;

        std::cout << std::left
                  << std::setw(10) << N
                  << std::setw(10) << finalStats.tradesGenerated
                  << std::setw(10) << numPriceLevels
                  << std::setw(10) << std::fixed << std::setprecision(1) << avgOrdersPerLevel
                  << std::setw(11) << std::fixed << std::setprecision(3) << avgUploadMs
                  << std::setw(11) << std::fixed << std::setprecision(3) << avgDecodeMs
                  << std::setw(11) << std::fixed << std::setprecision(3) << avgClassMs
                  << std::setw(11) << std::fixed << std::setprecision(3) << avgPBuildMs
                  << std::setw(11) << std::fixed << std::setprecision(3) << avgMatchMs
                  << std::setw(11) << std::fixed << std::setprecision(3) << avgDnloadMs
                  << std::setw(11) << std::fixed << std::setprecision(3) << avgVerifyMs
                  << std::setw(11) << std::fixed << std::setprecision(3) << avgTotalMs
                  << std::setw(13) << std::fixed << std::setprecision(2) << mOrdersPerSec
                  << std::setw(13) << std::fixed << std::setprecision(2) << mTradesPerSec
                  << std::setw(14) << std::fixed << std::setprecision(2) << effectiveGBps
                  << '\n';
    }
    std::cout << '\n';
}

// -----------------------------------------------------------------------------
// Main Entrypoint
// -----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    printSystemInfo();

    bool releaseMode = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--release") == 0)
        {
            releaseMode = true;
        }
    }

    std::vector<std::size_t> batchSizes;
    if (releaseMode)
    {
        std::cout << "Configuration      : RELEASE Mode (1K, 10K, 100K, 1M, 10M)\n\n";
        batchSizes = {1000, 10000, 100000, 1000000, 10000000};
    }
    else
    {
        std::cout << "Configuration      : DEVELOPMENT Mode (1K, 10K, 100K, 1M) [Use --release for 10M]\n\n";
        batchSizes = {1000, 10000, 100000, 1000000};
    }

    try
    {
        // Default scenario is Scenario C (Mixed Market) as requested
        runBenchmark(Scenario::MixedMarket, batchSizes);
        runBenchmark(Scenario::NoMatch, batchSizes);
        runBenchmark(Scenario::FullMatch, batchSizes);

        std::cout << "=========================================================================================================================================\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal Error: GPUMatchingEngine benchmark failed: " << ex.what() << '\n';
        return 1;
    }
}
