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
#include <map>
#include <random>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

struct CpuPriceLevel
{
    uint32_t priceTick;
    uint8_t side;
    int totalQuantity = 0;
    int orderCount = 0;
    std::vector<int> addIndices;
};

/**
 * @brief Helper to generate synthetic events with controlled probabilities and duplicate prices.
 */
std::vector<Event> generateEventsWithPriceDuplicates(
    std::size_t n,
    std::size_t numUniquePrices = 20,
    uint32_t seed = 42)
{
    std::mt19937 rng(seed);
    std::uniform_int_distribution<uint64_t> timeDist(100000, 9999999);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_int_distribution<int> priceIdxDist(0, static_cast<int>(numUniquePrices - 1));
    std::uniform_int_distribution<int> qtyDist(10, 500);

    std::vector<double> fixedPrices(numUniquePrices);
    for (std::size_t i = 0; i < numUniquePrices; ++i)
    {
        fixedPrices[i] = 100.0 + static_cast<double>(i) * 0.50; // $100.00, $100.50, $101.00...
    }

    std::vector<Event> events(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        events[i].timestamp = timeDist(rng);
        events[i].type = EventType::Add; // 100% Add events for price level testing
        events[i].orderId = static_cast<int>(i + 1);
        events[i].order.id = events[i].orderId;
        events[i].order.side = static_cast<Side>(sideDist(rng));
        events[i].order.type = OrderType::Limit;
        events[i].order.price = fixedPrices[priceIdxDist(rng)];
        events[i].order.quantity = qtyDist(rng);
        events[i].order.timestamp = events[i].timestamp;
        events[i].order.displayQuantity = events[i].order.quantity;
        events[i].order.reserveQuantity = 0;
    }

    return events;
}

/**
 * @brief Test 1: PriceLevelBuffer default & capacity construction.
 */
bool testBufferConstruction()
{
    PriceLevelBuffer emptyBuf;
    if (!emptyBuf.empty() || emptyBuf.size() != 0 || emptyBuf.capacity() != 0 || emptyBuf.isValid())
    {
        std::cerr << "  Failure: Default constructor invalid state\n";
        return false;
    }

    constexpr std::size_t cap = 1024;
    PriceLevelBuffer capBuf(cap);
    if (!capBuf.empty() || capBuf.size() != 0 || capBuf.capacity() != cap || !capBuf.isValid())
    {
        std::cerr << "  Failure: Capacity constructor invalid state\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 2: Price level aggregation & uint32_t price tick verification.
 */
bool testPriceLevelAggregation()
{
    CUDAContext context;
    constexpr std::size_t N = 1000;
    constexpr double tickSize = 0.01;
    std::vector<Event> h_events = generateEventsWithPriceDuplicates(N, 10, 123);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer decodedBuf(N);
    decodeEvents(replayBuf, decodedBuf, context);

    ClassifiedEventBuffer classifiedBuf(N);
    classifyEvents(decodedBuf, classifiedBuf, context);

    PriceLevelBuffer levelBuf;
    buildPriceLevels(decodedBuf, classifiedBuf, levelBuf, context, tickSize);

    if (levelBuf.empty() || levelBuf.size() == 0)
    {
        std::cerr << "  Failure: PriceLevelBuffer is empty after build\n";
        return false;
    }

    // CPU reference aggregation
    std::map<std::pair<uint8_t, uint32_t>, CpuPriceLevel> cpuLevels;
    for (std::size_t i = 0; i < N; ++i)
    {
        uint8_t side = static_cast<uint8_t>(h_events[i].order.side);
        uint32_t tick = static_cast<uint32_t>(std::llround(h_events[i].order.price / tickSize));
        auto key = std::make_pair(side, tick);

        auto& lvl = cpuLevels[key];
        lvl.side = side;
        lvl.priceTick = tick;
        lvl.totalQuantity += h_events[i].order.quantity;
        lvl.orderCount++;
        lvl.addIndices.push_back(static_cast<int>(i));
    }

    if (levelBuf.size() != cpuLevels.size())
    {
        std::cerr << "  Failure: Unique price level count mismatch (GPU: "
                  << levelBuf.size() << ", CPU: " << cpuLevels.size() << ")\n";
        return false;
    }

    // Download GPU levels
    std::vector<PriceLevel> h_gpuLevels(levelBuf.size());
    levelBuf.getLevelsBuffer().copyToHostAsync(h_gpuLevels.data(), levelBuf.size(), context.getStream());
    context.synchronize();

    for (const auto& gpuLvl : h_gpuLevels)
    {
        auto key = std::make_pair(gpuLvl.side, gpuLvl.priceTick);
        if (cpuLevels.find(key) == cpuLevels.end())
        {
            std::cerr << "  Failure: GPU produced level (side " << static_cast<int>(gpuLvl.side)
                      << ", tick " << gpuLvl.priceTick << ") not found on CPU\n";
            return false;
        }

        const auto& cpuLvl = cpuLevels[key];
        if (gpuLvl.totalQuantity != cpuLvl.totalQuantity)
        {
            std::cerr << "  Failure: Quantity mismatch for tick " << gpuLvl.priceTick
                      << " (GPU: " << gpuLvl.totalQuantity << ", CPU: " << cpuLvl.totalQuantity << ")\n";
            return false;
        }

        if (gpuLvl.orderCount != cpuLvl.orderCount)
        {
            std::cerr << "  Failure: Order count mismatch for tick " << gpuLvl.priceTick
                      << " (GPU: " << gpuLvl.orderCount << ", CPU: " << cpuLvl.orderCount << ")\n";
            return false;
        }
    }

    return true;
}

/**
 * @brief Test 3: Distinct Buy vs. Sell price levels at the exact same numerical price.
 */
bool testDistinctBuySellAtSamePrice()
{
    CUDAContext context;
    constexpr double tickSize = 0.01;
    constexpr double targetPrice = 150.00;
    const uint32_t expectedTick = static_cast<uint32_t>(std::llround(targetPrice / tickSize));

    std::vector<Event> h_events(4);
    // Buy order 1
    h_events[0].type = EventType::Add;
    h_events[0].order.side = Side::Buy;
    h_events[0].order.price = targetPrice;
    h_events[0].order.quantity = 100;

    // Buy order 2
    h_events[1].type = EventType::Add;
    h_events[1].order.side = Side::Buy;
    h_events[1].order.price = targetPrice;
    h_events[1].order.quantity = 200;

    // Sell order 1
    h_events[2].type = EventType::Add;
    h_events[2].order.side = Side::Sell;
    h_events[2].order.price = targetPrice;
    h_events[2].order.quantity = 300;

    // Sell order 2
    h_events[3].type = EventType::Add;
    h_events[3].order.side = Side::Sell;
    h_events[3].order.price = targetPrice;
    h_events[3].order.quantity = 400;

    ReplayBuffer replayBuf(4);
    replayBuf.uploadBatch(h_events.data(), 4, context.getStream());

    DecodedEventBuffer decodedBuf(4);
    decodeEvents(replayBuf, decodedBuf, context);

    ClassifiedEventBuffer classifiedBuf(4);
    classifyEvents(decodedBuf, classifiedBuf, context);

    PriceLevelBuffer levelBuf;
    buildPriceLevels(decodedBuf, classifiedBuf, levelBuf, context, tickSize);

    if (levelBuf.size() != 2)
    {
        std::cerr << "  Failure: Expected 2 distinct levels (1 Buy, 1 Sell), got " << levelBuf.size() << '\n';
        return false;
    }

    std::vector<PriceLevel> h_gpuLevels(2);
    levelBuf.getLevelsBuffer().copyToHostAsync(h_gpuLevels.data(), 2, context.getStream());
    context.synchronize();

    bool foundBuy = false;
    bool foundSell = false;

    for (const auto& lvl : h_gpuLevels)
    {
        if (lvl.priceTick != expectedTick)
        {
            std::cerr << "  Failure: Unexpected price tick " << lvl.priceTick << " (expected " << expectedTick << ")\n";
            return false;
        }

        if (lvl.side == static_cast<uint8_t>(Side::Buy))
        {
            foundBuy = true;
            if (lvl.totalQuantity != 300 || lvl.orderCount != 2)
            {
                std::cerr << "  Failure: Buy level quantity/count mismatch\n";
                return false;
            }
        }
        else if (lvl.side == static_cast<uint8_t>(Side::Sell))
        {
            foundSell = true;
            if (lvl.totalQuantity != 700 || lvl.orderCount != 2)
            {
                std::cerr << "  Failure: Sell level quantity/count mismatch\n";
                return false;
            }
        }
    }

    if (!foundBuy || !foundSell)
    {
        std::cerr << "  Failure: Did not find both Buy and Sell price levels\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 4: CSR mapping preservation (firstOrder, orderCount).
 */
bool testCSRMappingPreservation()
{
    CUDAContext context;
    constexpr std::size_t N = 500;
    constexpr double tickSize = 0.01;
    std::vector<Event> h_events = generateEventsWithPriceDuplicates(N, 5, 456);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer decodedBuf(N);
    decodeEvents(replayBuf, decodedBuf, context);

    ClassifiedEventBuffer classifiedBuf(N);
    classifyEvents(decodedBuf, classifiedBuf, context);

    PriceLevelBuffer levelBuf;
    buildPriceLevels(decodedBuf, classifiedBuf, levelBuf, context, tickSize);

    std::vector<PriceLevel> h_levels(levelBuf.size());
    std::vector<int> h_orderIndices(N);

    levelBuf.getLevelsBuffer().copyToHostAsync(h_levels.data(), levelBuf.size(), context.getStream());
    levelBuf.getOrderIndicesBuffer().copyToHostAsync(h_orderIndices.data(), N, context.getStream());
    context.synchronize();

    for (const auto& lvl : h_levels)
    {
        if (lvl.firstOrder < 0 || static_cast<std::size_t>(lvl.firstOrder + lvl.orderCount) > N)
        {
            std::cerr << "  Failure: Invalid CSR offset range [" << lvl.firstOrder
                      << ", " << (lvl.firstOrder + lvl.orderCount) << ")\n";
            return false;
        }

        int accumulatedQty = 0;
        for (int i = 0; i < lvl.orderCount; ++i)
        {
            int orderIdx = h_orderIndices[lvl.firstOrder + i];
            if (orderIdx < 0 || static_cast<std::size_t>(orderIdx) >= N)
            {
                std::cerr << "  Failure: Out-of-bounds order index " << orderIdx << '\n';
                return false;
            }

            uint32_t orderTick = static_cast<uint32_t>(std::llround(h_events[orderIdx].order.price / tickSize));
            uint8_t orderSide = static_cast<uint8_t>(h_events[orderIdx].order.side);

            if (orderTick != lvl.priceTick || orderSide != lvl.side)
            {
                std::cerr << "  Failure: CSR order index " << orderIdx << " attribute mismatch\n";
                return false;
            }

            accumulatedQty += h_events[orderIdx].order.quantity;
        }

        if (accumulatedQty != lvl.totalQuantity)
        {
            std::cerr << "  Failure: CSR accumulated quantity " << accumulatedQty
                      << " != level totalQuantity " << lvl.totalQuantity << '\n';
            return false;
        }
    }

    return true;
}

/**
 * @brief Test 5: Empty input batch (0 events).
 */
bool testEmptyInput()
{
    CUDAContext context;
    DecodedEventBuffer emptyDecoded;
    ClassifiedEventBuffer emptyClassified;
    PriceLevelBuffer levelBuf(100);

    buildPriceLevels(emptyDecoded, emptyClassified, levelBuf, context);

    if (levelBuf.size() != 0 || !levelBuf.empty())
    {
        std::cerr << "  Failure: Empty input did not set price level size to 0\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 6: Move constructor and move assignment for PriceLevelBuffer.
 */
bool testMoveSemantics()
{
    CUDAContext context;
    constexpr std::size_t N = 1000;
    std::vector<Event> h_events = generateEventsWithPriceDuplicates(N, 10, 888);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer decodedBuf(N);
    decodeEvents(replayBuf, decodedBuf, context);

    ClassifiedEventBuffer classifiedBuf(N);
    classifyEvents(decodedBuf, classifiedBuf, context);

    PriceLevelBuffer srcBuf;
    buildPriceLevels(decodedBuf, classifiedBuf, srcBuf, context);
    std::size_t expectedSize = srcBuf.size();

    // Move constructor
    PriceLevelBuffer dstBuf(std::move(srcBuf));
    if (dstBuf.size() != expectedSize || !dstBuf.isValid())
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
    PriceLevelBuffer dstBuf2;
    dstBuf2 = std::move(dstBuf);
    if (dstBuf2.size() != expectedSize || !dstBuf2.isValid())
    {
        std::cerr << "  Failure: Move assignment failed to transfer state\n";
        return false;
    }
    if (dstBuf.size() != 0 || dstBuf.capacity() != 0 || dstBuf.isValid())
    {
        std::cerr << "  Failure: Move source was not cleanly reset after move assignment\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 7: Invalid arguments exception handling.
 */
bool testInvalidArguments()
{
    CUDAContext context;
    DecodedEventBuffer decodedBuf;
    ClassifiedEventBuffer classifiedBuf;
    PriceLevelBuffer levelBuf(100);

    bool tickSizeThrew = false;
    try
    {
        buildPriceLevels(decodedBuf, classifiedBuf, levelBuf, context, -0.01);
    }
    catch (const std::invalid_argument&)
    {
        tickSizeThrew = true;
    }

    if (!tickSizeThrew)
    {
        std::cerr << "  Failure: Negative tickSize did not throw std::invalid_argument\n";
        return false;
    }

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
        {"2. Price Level Aggregation & Tick Verification", testPriceLevelAggregation},
        {"3. Distinct Buy vs. Sell Levels at Same Price", testDistinctBuySellAtSamePrice},
        {"4. CSR Order-to-Level Mapping Preservation", testCSRMappingPreservation},
        {"5. Empty Input Batch (0 Events)", testEmptyInput},
        {"6. Move Semantics (Constructor & Assignment)", testMoveSemantics},
        {"7. Invalid Arguments Exception Handling", testInvalidArguments}
    };

    std::cout << "========================================\n";
    std::cout << "  PriceLevelBuilder Correctness Tests   \n";
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
