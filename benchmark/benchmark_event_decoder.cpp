#include "cuda/EventDecoder.h"
#include "cuda/ReplayBuffer.h"
#include "cuda/DecodedEventBuffer.h"
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

/**
 * @brief CPU reference implementation for decoding AoS Event array into SoA vectors.
 */
struct CPUDecodedEventSoA
{
    std::vector<uint64_t> timestamps;
    std::vector<uint8_t> eventTypes;
    std::vector<int> orderIds;
    std::vector<uint8_t> sides;
    std::vector<uint8_t> orderTypes;
    std::vector<double> prices;
    std::vector<int> quantities;
    std::vector<int> displayQuantities;
    std::vector<int> reserveQuantities;

    explicit CPUDecodedEventSoA(std::size_t n)
        : timestamps(n), eventTypes(n), orderIds(n), sides(n), orderTypes(n),
          prices(n), quantities(n), displayQuantities(n), reserveQuantities(n)
    {}
};

CPUDecodedEventSoA cpuDecodeEvents(const std::vector<Event>& events)
{
    const std::size_t n = events.size();
    CPUDecodedEventSoA cpuSoA(n);

    for (std::size_t i = 0; i < n; ++i)
    {
        const Event& ev = events[i];
        cpuSoA.timestamps[i] = ev.timestamp;
        cpuSoA.eventTypes[i] = static_cast<uint8_t>(ev.type);
        cpuSoA.orderIds[i] = ev.orderId;
        cpuSoA.sides[i] = static_cast<uint8_t>(ev.order.side);
        cpuSoA.orderTypes[i] = static_cast<uint8_t>(ev.order.type);
        cpuSoA.prices[i] = ev.order.price;
        cpuSoA.quantities[i] = ev.order.quantity;
        cpuSoA.displayQuantities[i] = ev.order.displayQuantity;
        cpuSoA.reserveQuantities[i] = ev.order.reserveQuantity;
    }

    return cpuSoA;
}

/**
 * @brief Downloads GPU DecodedEventBuffer data into host vectors for accuracy verification.
 */
CPUDecodedEventSoA downloadDecodedBuffer(const DecodedEventBuffer& gpuBuffer, const CUDAContext& context)
{
    const std::size_t n = gpuBuffer.size();
    CPUDecodedEventSoA hostSoA(n);

    if (n > 0)
    {
        gpuBuffer.timestampsBuffer().copyToHostAsync(hostSoA.timestamps.data(), n, context.getStream());
        gpuBuffer.eventTypesBuffer().copyToHostAsync(hostSoA.eventTypes.data(), n, context.getStream());
        gpuBuffer.orderIdsBuffer().copyToHostAsync(hostSoA.orderIds.data(), n, context.getStream());
        gpuBuffer.sidesBuffer().copyToHostAsync(hostSoA.sides.data(), n, context.getStream());
        gpuBuffer.orderTypesBuffer().copyToHostAsync(hostSoA.orderTypes.data(), n, context.getStream());
        gpuBuffer.pricesBuffer().copyToHostAsync(hostSoA.prices.data(), n, context.getStream());
        gpuBuffer.quantitiesBuffer().copyToHostAsync(hostSoA.quantities.data(), n, context.getStream());
        gpuBuffer.displayQuantitiesBuffer().copyToHostAsync(hostSoA.displayQuantities.data(), n, context.getStream());
        gpuBuffer.reserveQuantitiesBuffer().copyToHostAsync(hostSoA.reserveQuantities.data(), n, context.getStream());
        context.synchronize();
    }

    return hostSoA;
}

/**
 * @brief Field-by-field verification helper.
 */
bool verifyDecodedFields(const CPUDecodedEventSoA& gpuDecoded, const CPUDecodedEventSoA& cpuRef)
{
    if (gpuDecoded.timestamps.size() != cpuRef.timestamps.size())
    {
        return false;
    }

    for (std::size_t i = 0; i < cpuRef.timestamps.size(); ++i)
    {
        if (gpuDecoded.timestamps[i] != cpuRef.timestamps[i] ||
            gpuDecoded.eventTypes[i] != cpuRef.eventTypes[i] ||
            gpuDecoded.orderIds[i] != cpuRef.orderIds[i] ||
            gpuDecoded.sides[i] != cpuRef.sides[i] ||
            gpuDecoded.orderTypes[i] != cpuRef.orderTypes[i] ||
            gpuDecoded.prices[i] != cpuRef.prices[i] ||
            gpuDecoded.quantities[i] != cpuRef.quantities[i] ||
            gpuDecoded.displayQuantities[i] != cpuRef.displayQuantities[i] ||
            gpuDecoded.reserveQuantities[i] != cpuRef.reserveQuantities[i])
        {
            std::cerr << "  Failure: Mismatch at benchmark index " << i << '\n';
            return false;
        }
    }

    return true;
}

int main()
{
    std::cout << "=============================================================================================================\n";
    std::cout << "                        GPU Event Decoder (AoS -> SoA) Performance Benchmark                        \n";
    std::cout << "=============================================================================================================\n";
    std::cout << "Size of Event (AoS): " << sizeof(Event) << " bytes | Size of Decoded Event (SoA): "
              << (sizeof(uint64_t) + sizeof(uint8_t) + sizeof(int) + sizeof(uint8_t) + sizeof(uint8_t) + sizeof(double) + sizeof(int)*3)
              << " bytes\n\n";

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
        std::cout << "-------------------------------------------------------------------------------------------------------------\n";

        std::mt19937 rng(42);
        std::uniform_int_distribution<uint64_t> timeDist(1000000, 9000000);
        std::uniform_int_distribution<int> typeDist(0, 6);
        std::uniform_int_distribution<int> sideDist(0, 1);
        std::uniform_int_distribution<int> orderTypeDist(0, 2);
        std::uniform_real_distribution<double> priceDist(10.0, 500.0);
        std::uniform_int_distribution<int> qtyDist(1, 1000);

        for (std::size_t N : batchSizes)
        {
            std::vector<Event> h_events(N);
            for (std::size_t i = 0; i < N; ++i)
            {
                h_events[i].timestamp = timeDist(rng);
                h_events[i].type = static_cast<EventType>(typeDist(rng));
                h_events[i].orderId = static_cast<int>(i + 1);
                h_events[i].order.id = h_events[i].orderId;
                h_events[i].order.side = static_cast<Side>(sideDist(rng));
                h_events[i].order.type = static_cast<OrderType>(orderTypeDist(rng));
                h_events[i].order.price = priceDist(rng);
                h_events[i].order.quantity = qtyDist(rng);
                h_events[i].order.timestamp = h_events[i].timestamp;
                h_events[i].order.displayQuantity = h_events[i].order.quantity;
                h_events[i].order.reserveQuantity = 0;
            }

            constexpr std::size_t soaEventBytes =
                sizeof(uint64_t) + sizeof(uint8_t) + sizeof(int) +
                sizeof(uint8_t)  + sizeof(uint8_t) + sizeof(double) +
                sizeof(int)      + sizeof(int)      + sizeof(int);

            const std::size_t totalBytesTransferred = N * (sizeof(Event) + soaEventBytes);
            const double dataSizeMB = static_cast<double>(totalBytesTransferred) / (1024.0 * 1024.0);
            const double dataSizeGB = static_cast<double>(totalBytesTransferred) / (1024.0 * 1024.0 * 1024.0);

            ReplayBuffer replayBuf(N);
            DecodedEventBuffer decodedBuf(N);

            // Warmup execution
            replayBuf.uploadBatch(h_events.data(), N, context.getStream());
            decodeEvents(replayBuf, decodedBuf, context);

            // --- End-to-End Timing (Host Upload + GPU Decode + Sync) ---
            constexpr int iterations = 10;
            auto t0_e2e = std::chrono::high_resolution_clock::now();

            for (int it = 0; it < iterations; ++it)
            {
                replayBuf.uploadBatch(h_events.data(), N, context.getStream());
                decodeEventsAsync(replayBuf, decodedBuf, context.getStream());
            }
            context.synchronize();

            auto t1_e2e = std::chrono::high_resolution_clock::now();
            double totalE2EMs = std::chrono::duration<double, std::milli>(t1_e2e - t0_e2e).count();
            double avgE2EMs = totalE2EMs / iterations;

            // --- Kernel Only Timing (CUDA Events) ---
            context.startTimer();
            for (int it = 0; it < iterations; ++it)
            {
                decodeEventsAsync(replayBuf, decodedBuf, context.getStream());
            }
            context.stopTimer();
            double totalKernelMs = context.elapsedMilliseconds();
            double avgKernelMs = totalKernelMs / iterations;

            double mEventsPerSec = (avgKernelMs > 0.0) ? ((static_cast<double>(N) / 1e6) / (avgKernelMs / 1000.0)) : 0.0;
            double effectiveGBps = (avgKernelMs > 0.0) ? (dataSizeGB / (avgKernelMs / 1000.0)) : 0.0;

            // Verification
            CPUDecodedEventSoA cpuRef = cpuDecodeEvents(h_events);
            CPUDecodedEventSoA gpuDecoded = downloadDecodedBuffer(decodedBuf, context);
            bool isCorrect = verifyDecodedFields(gpuDecoded, cpuRef);

            std::cout << std::left
                      << std::setw(12) << N
                      << std::setw(14) << std::fixed << std::setprecision(2) << dataSizeMB
                      << std::setw(16) << std::fixed << std::setprecision(3) << avgKernelMs
                      << std::setw(16) << std::fixed << std::setprecision(3) << avgE2EMs
                      << std::setw(16) << std::fixed << std::setprecision(2) << mEventsPerSec
                      << std::setw(18) << std::fixed << std::setprecision(2) << effectiveGBps
                      << std::setw(12) << (isCorrect ? "✓ PASS" : "✗ FAIL")
                      << '\n';
        }

        std::cout << "=============================================================================================================\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Fatal Error: EventDecoder benchmark failed: " << ex.what() << '\n';
        return 1;
    }
}
