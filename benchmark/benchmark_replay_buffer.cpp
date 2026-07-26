#include "cuda/ReplayBuffer.h"
#include "cuda/CUDAContext.h"
#include "replay/Event.h"

#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <stdexcept>

/**
 * @brief Helper to validate downloaded Event data against source Event data.
 */
bool verifyEventBatch(const std::vector<Event>& h_src, const std::vector<Event>& h_dst)
{
    if (h_src.size() != h_dst.size())
    {
        return false;
    }

    for (std::size_t i = 0; i < h_src.size(); ++i)
    {
        if (h_src[i].timestamp != h_dst[i].timestamp ||
            h_src[i].type != h_dst[i].type ||
            h_src[i].orderId != h_dst[i].orderId ||
            h_src[i].order.price != h_dst[i].order.price ||
            h_src[i].order.quantity != h_dst[i].order.quantity)
        {
            std::cerr << "  Failure: Mismatch at benchmark index " << i << '\n';
            return false;
        }
    }

    return true;
}

int main()
{
    std::cout << "========================================================================================\n";
    std::cout << "                     GPU ReplayBuffer Upload/Download Throughput                        \n";
    std::cout << "========================================================================================\n";
    std::cout << "Size of Event structure: " << sizeof(Event) << " bytes\n\n";

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
                  << std::setw(16) << "Upload Time(ms)"
                  << std::setw(16) << "Upload (GB/s)"
                  << std::setw(18) << "Download Time(ms)"
                  << std::setw(18) << "Download (GB/s)"
                  << std::setw(12) << "Verified"
                  << '\n';
        std::cout << "----------------------------------------------------------------------------------------\n";

        std::mt19937 rng(42);
        std::uniform_int_distribution<uint64_t> timeDist(1000000, 9000000);
        std::uniform_real_distribution<double> priceDist(10.0, 500.0);
        std::uniform_int_distribution<int> qtyDist(1, 1000);

        for (std::size_t N : batchSizes)
        {
            std::vector<Event> h_src(N);
            for (std::size_t i = 0; i < N; ++i)
            {
                h_src[i].timestamp = timeDist(rng);
                h_src[i].type = (i % 2 == 0) ? EventType::Add : EventType::Cancel;
                h_src[i].orderId = static_cast<int>(i + 1);
                h_src[i].order.id = h_src[i].orderId;
                h_src[i].order.side = (i % 3 == 0) ? Side::Buy : Side::Sell;
                h_src[i].order.type = OrderType::Limit;
                h_src[i].order.price = priceDist(rng);
                h_src[i].order.quantity = qtyDist(rng);
                h_src[i].order.timestamp = h_src[i].timestamp;
                h_src[i].order.displayQuantity = h_src[i].order.quantity;
                h_src[i].order.reserveQuantity = 0;
            }

            const double dataSizeMB = static_cast<double>(N * sizeof(Event)) / (1024.0 * 1024.0);
            const double dataSizeGB = static_cast<double>(N * sizeof(Event)) / (1024.0 * 1024.0 * 1024.0);

            ReplayBuffer replayBuf(N);
            std::vector<Event> h_dst(N);

            // Warmup transfer
            replayBuf.uploadBatch(h_src.data(), N, context.getStream());
            context.synchronize();

            // Benchmark Upload
            constexpr int iterations = 5;
            auto t0_up = std::chrono::high_resolution_clock::now();
            for (int it = 0; it < iterations; ++it)
            {
                replayBuf.uploadBatch(h_src.data(), N, context.getStream());
            }
            context.synchronize();
            auto t1_up = std::chrono::high_resolution_clock::now();

            double totalUpMs = std::chrono::duration<double, std::milli>(t1_up - t0_up).count();
            double avgUpMs = totalUpMs / iterations;
            double uploadGBps = (avgUpMs > 0.0) ? (dataSizeGB / (avgUpMs / 1000.0)) : 0.0;

            // Benchmark Download
            auto t0_dn = std::chrono::high_resolution_clock::now();
            for (int it = 0; it < iterations; ++it)
            {
                replayBuf.downloadBatch(h_dst.data(), N, context.getStream());
            }
            context.synchronize();
            auto t1_dn = std::chrono::high_resolution_clock::now();

            double totalDnMs = std::chrono::duration<double, std::milli>(t1_dn - t0_dn).count();
            double avgDnMs = totalDnMs / iterations;
            double downloadGBps = (avgDnMs > 0.0) ? (dataSizeGB / (avgDnMs / 1000.0)) : 0.0;

            bool isCorrect = verifyEventBatch(h_src, h_dst);

            std::cout << std::left
                      << std::setw(12) << N
                      << std::setw(14) << std::fixed << std::setprecision(2) << dataSizeMB
                      << std::setw(16) << std::fixed << std::setprecision(3) << avgUpMs
                      << std::setw(16) << std::fixed << std::setprecision(2) << uploadGBps
                      << std::setw(18) << std::fixed << std::setprecision(3) << avgDnMs
                      << std::setw(18) << std::fixed << std::setprecision(2) << downloadGBps
                      << std::setw(12) << (isCorrect ? "✓ PASS" : "✗ FAIL")
                      << '\n';
        }

        std::cout << "========================================================================================\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal Error: ReplayBuffer benchmark failed: " << ex.what() << '\n';
        return 1;
    }
}
