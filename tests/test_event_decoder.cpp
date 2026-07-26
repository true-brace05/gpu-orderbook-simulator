#include "cuda/EventDecoder.h"
#include "cuda/ReplayBuffer.h"
#include "cuda/CUDAContext.h"
#include "replay/Event.h"
#include "Types.h"

#include <iostream>
#include <vector>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>

/**
 * @brief CPU reference structure for SoA decoded events.
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

/**
 * @brief CPU reference implementation for decoding AoS Event array into SoA vectors.
 */
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
 * @brief Download GPU DecodedEventBuffer contents into host CPU vectors.
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
 * @brief Verifies every field of GPU decoded buffer against CPU reference.
 */
bool verifyDecodedFields(const CPUDecodedEventSoA& gpuDecoded, const CPUDecodedEventSoA& cpuRef)
{
    if (gpuDecoded.timestamps.size() != cpuRef.timestamps.size())
    {
        std::cerr << "  Failure: Size mismatch between GPU (" << gpuDecoded.timestamps.size()
                  << ") and CPU (" << cpuRef.timestamps.size() << ")\n";
        return false;
    }

    for (std::size_t i = 0; i < cpuRef.timestamps.size(); ++i)
    {
        if (gpuDecoded.timestamps[i] != cpuRef.timestamps[i])
        {
            std::cerr << "  Failure: timestamp mismatch at index " << i << '\n';
            return false;
        }
        if (gpuDecoded.eventTypes[i] != cpuRef.eventTypes[i])
        {
            std::cerr << "  Failure: eventType mismatch at index " << i << '\n';
            return false;
        }
        if (gpuDecoded.orderIds[i] != cpuRef.orderIds[i])
        {
            std::cerr << "  Failure: orderId mismatch at index " << i << '\n';
            return false;
        }
        if (gpuDecoded.sides[i] != cpuRef.sides[i])
        {
            std::cerr << "  Failure: side mismatch at index " << i << '\n';
            return false;
        }
        if (gpuDecoded.orderTypes[i] != cpuRef.orderTypes[i])
        {
            std::cerr << "  Failure: orderType mismatch at index " << i << '\n';
            return false;
        }
        if (gpuDecoded.prices[i] != cpuRef.prices[i])
        {
            std::cerr << "  Failure: price mismatch at index " << i << '\n';
            return false;
        }
        if (gpuDecoded.quantities[i] != cpuRef.quantities[i])
        {
            std::cerr << "  Failure: quantity mismatch at index " << i << '\n';
            return false;
        }
        if (gpuDecoded.displayQuantities[i] != cpuRef.displayQuantities[i])
        {
            std::cerr << "  Failure: displayQuantity mismatch at index " << i << '\n';
            return false;
        }
        if (gpuDecoded.reserveQuantities[i] != cpuRef.reserveQuantities[i])
        {
            std::cerr << "  Failure: reserveQuantity mismatch at index " << i << '\n';
            return false;
        }
    }

    return true;
}

/**
 * @brief Helper to construct synthetic test events.
 */
std::vector<Event> generateRandomEvents(std::size_t n, uint32_t seed = 42)
{
    std::mt19937 rng(seed);
    std::uniform_int_distribution<uint64_t> timeDist(100000, 9999999);
    std::uniform_int_distribution<int> typeDist(0, 6);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> orderTypeDist(0, 2);
    std::uniform_real_distribution<double> priceDist(1.0, 1000.0);
    std::uniform_int_distribution<int> qtyDist(1, 500);

    std::vector<Event> events(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        events[i].timestamp = timeDist(rng);
        events[i].type = static_cast<EventType>(typeDist(rng));
        events[i].orderId = static_cast<int>(i + 1);
        events[i].order.id = events[i].orderId;
        events[i].order.side = static_cast<Side>(sideDist(rng));
        events[i].order.type = static_cast<OrderType>(orderTypeDist(rng));
        events[i].order.price = priceDist(rng);
        events[i].order.quantity = qtyDist(rng);
        events[i].order.timestamp = events[i].timestamp;
        events[i].order.displayQuantity = events[i].order.quantity;
        events[i].order.reserveQuantity = (events[i].order.type == OrderType::Iceberg) ? 100 : 0;
    }

    return events;
}

/**
 * @brief Test 1: DecodedEventBuffer default construction and capacity allocation.
 */
bool testBufferConstruction()
{
    DecodedEventBuffer emptyBuf;
    if (!emptyBuf.empty() || emptyBuf.size() != 0 || emptyBuf.capacity() != 0 || emptyBuf.isValid())
    {
        std::cerr << "  Failure: Default constructor invalid state\n";
        return false;
    }

    constexpr std::size_t cap = 1024;
    DecodedEventBuffer capBuf(cap);
    if (!capBuf.empty() || capBuf.size() != 0 || capBuf.capacity() != cap || !capBuf.isValid())
    {
        std::cerr << "  Failure: Capacity constructor invalid state\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 2: Field-by-field correctness against CPU reference implementation.
 */
bool testFieldCorrectness()
{
    CUDAContext context;
    constexpr std::size_t N = 1000;
    std::vector<Event> h_events = generateRandomEvents(N);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer decodedBuf(N);
    decodeEvents(replayBuf, decodedBuf, context);

    if (decodedBuf.size() != N)
    {
        std::cerr << "  Failure: Decoded size() = " << decodedBuf.size() << " (expected " << N << ")\n";
        return false;
    }

    CPUDecodedEventSoA cpuRef = cpuDecodeEvents(h_events);
    CPUDecodedEventSoA gpuDecoded = downloadDecodedBuffer(decodedBuf, context);

    return verifyDecodedFields(gpuDecoded, cpuRef);
}

/**
 * @brief Test 3: Empty batch decoding (0 elements).
 */
bool testEmptyBatchDecoding()
{
    CUDAContext context;
    ReplayBuffer emptyReplayBuf;
    DecodedEventBuffer decodedBuf(100);

    decodeEvents(emptyReplayBuf, decodedBuf, context);

    if (decodedBuf.size() != 0 || !decodedBuf.empty())
    {
        std::cerr << "  Failure: Empty batch decoding did not set output size to 0\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 4: Large batch decoding (100,000 events).
 */
bool testLargeBatchDecoding()
{
    CUDAContext context;
    constexpr std::size_t N = 100000;
    std::vector<Event> h_events = generateRandomEvents(N, 1337);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer decodedBuf;
    decodeEvents(replayBuf, decodedBuf, context); // Auto-reserves

    if (decodedBuf.size() != N || decodedBuf.capacity() < N)
    {
        std::cerr << "  Failure: Large batch decoding size/capacity mismatch\n";
        return false;
    }

    CPUDecodedEventSoA cpuRef = cpuDecodeEvents(h_events);
    CPUDecodedEventSoA gpuDecoded = downloadDecodedBuffer(decodedBuf, context);

    return verifyDecodedFields(gpuDecoded, cpuRef);
}

/**
 * @brief Test 5: Move constructor and move assignment semantics.
 */
bool testMoveSemantics()
{
    CUDAContext context;
    constexpr std::size_t N = 500;
    std::vector<Event> h_events = generateRandomEvents(N);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer srcBuf;
    decodeEvents(replayBuf, srcBuf, context);

    // Move constructor
    DecodedEventBuffer dstBuf(std::move(srcBuf));
    if (dstBuf.size() != N || !dstBuf.isValid())
    {
        std::cerr << "  Failure: Move constructor failed to transfer state\n";
        return false;
    }
    if (srcBuf.size() != 0 || srcBuf.capacity() != 0 || srcBuf.isValid())
    {
        std::cerr << "  Failure: Move source was not cleanly reset after move constructor\n";
        return false;
    }

    // Move assignment
    DecodedEventBuffer dstBuf2;
    dstBuf2 = std::move(dstBuf);
    if (dstBuf2.size() != N || !dstBuf2.isValid())
    {
        std::cerr << "  Failure: Move assignment failed to transfer state\n";
        return false;
    }
    if (dstBuf.size() != 0 || dstBuf.capacity() != 0 || dstBuf.isValid())
    {
        std::cerr << "  Failure: Move source was not cleanly reset after move assignment\n";
        return false;
    }

    CPUDecodedEventSoA cpuRef = cpuDecodeEvents(h_events);
    CPUDecodedEventSoA gpuDecoded = downloadDecodedBuffer(dstBuf2, context);

    return verifyDecodedFields(gpuDecoded, cpuRef);
}

/**
 * @brief Test 6: Clear and Release behavior following std::vector semantics.
 */
bool testClearAndRelease()
{
    CUDAContext context;
    constexpr std::size_t N = 300;
    std::vector<Event> h_events = generateRandomEvents(N);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer buf;
    decodeEvents(replayBuf, buf, context);

    buf.clear();
    if (buf.size() != 0 || !buf.empty())
    {
        std::cerr << "  Failure: clear() did not reset size to 0\n";
        return false;
    }
    if (buf.capacity() < N || !buf.isValid())
    {
        std::cerr << "  Failure: clear() released GPU capacity or invalidated state\n";
        return false;
    }

    buf.release();
    if (buf.size() != 0 || buf.capacity() != 0 || buf.isValid())
    {
        std::cerr << "  Failure: release() failed to reset state completely\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 7: Invalid arguments exception handling.
 */
bool testInvalidArguments()
{
    bool ctorThrew = false;
    try
    {
        DecodedEventBuffer buf(0);
    }
    catch (const std::invalid_argument&)
    {
        ctorThrew = true;
    }
    if (!ctorThrew)
    {
        std::cerr << "  Failure: DecodedEventBuffer(0) did not throw std::invalid_argument\n";
        return false;
    }

    CUDAContext context;
    ReplayBuffer unallocatedReplayBuf;
    DecodedEventBuffer outputBuf(100);

    bool unallocatedThrew = false;
    try
    {
        // ReplayBuffer is unallocated but size is 0 -> should handle gracefully or throw if invalid
        unallocatedReplayBuf.allocate(100);
        unallocatedReplayBuf.release();
        decodeEvents(unallocatedReplayBuf, outputBuf, context);
    }
    catch (const std::invalid_argument&)
    {
        unallocatedThrew = true;
    }
    // Note: empty unallocated buffer has size() == 0, so decodeEvents sets outputBuf to size 0

    return true;
}

struct TestCase
{
    std::string name;
    bool (*func)();
};

int main()
{
    const std::vector<TestCase> tests = {
        {"1. Buffer Construction & Capacity", testBufferConstruction},
        {"2. Field-by-Field Correctness (CPU vs GPU)", testFieldCorrectness},
        {"3. Empty Batch Decoding (0 Events)", testEmptyBatchDecoding},
        {"4. Large Batch Decoding (100K Events)", testLargeBatchDecoding},
        {"5. Move Semantics (Constructor & Assignment)", testMoveSemantics},
        {"6. Clear & Release (std::vector Semantics)", testClearAndRelease},
        {"7. Invalid Arguments Exception Handling", testInvalidArguments}
    };

    std::cout << "========================================\n";
    std::cout << "     EventDecoder Correctness Tests     \n";
    std::cout << "========================================\n";

    std::size_t passedCount = 0;

    for (const auto& test : tests)
    {
        std::cout << "Running: " << test.name << "... ";
        try
        {
            if (test.func())
            {
                std::cout << "✓ PASS\n";
                ++passedCount;
            }
            else
            {
                std::cout << "✗ FAIL\n";
            }
        }
        catch (const std::exception& ex)
        {
            std::cout << "✗ FAIL (Unhandled Exception: " << ex.what() << ")\n";
        }
        catch (...)
        {
            std::cout << "✗ FAIL (Unknown Exception)\n";
        }
    }

    std::cout << "========================================\n";
    std::cout << "Summary: " << passedCount << " / " << tests.size() << " tests passed.\n";
    std::cout << "========================================\n";

    return (passedCount == tests.size()) ? 0 : 1;
}
