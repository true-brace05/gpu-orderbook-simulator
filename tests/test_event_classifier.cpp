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
#include <algorithm>
#include <unordered_set>
#include <stdexcept>
#include <string>
#include <utility>

/**
 * @brief Helper to generate synthetic events with controlled probabilities for each event type.
 */
std::vector<Event> generateEventsWithDistribution(
    std::size_t n,
    const std::vector<double>& categoryWeights,
    uint32_t seed = 42)
{
    std::mt19937 rng(seed);
    std::discrete_distribution<int> dist(categoryWeights.begin(), categoryWeights.end());
    std::uniform_int_distribution<uint64_t> timeDist(100000, 9999999);
    std::uniform_int_distribution<int> sideDist(0, 1);
    std::uniform_real_distribution<double> priceDist(1.0, 1000.0);
    std::uniform_int_distribution<int> qtyDist(1, 500);

    std::vector<Event> events(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        int cat = dist(rng);
        events[i].timestamp = timeDist(rng);
        events[i].type = static_cast<EventType>(cat);
        events[i].orderId = static_cast<int>(i + 1);
        events[i].order.id = events[i].orderId;
        events[i].order.side = static_cast<Side>(sideDist(rng));
        events[i].order.type = OrderType::Limit;
        events[i].order.price = priceDist(rng);
        events[i].order.quantity = qtyDist(rng);
        events[i].order.timestamp = events[i].timestamp;
        events[i].order.displayQuantity = events[i].order.quantity;
        events[i].order.reserveQuantity = 0;
    }

    return events;
}

/**
 * @brief Downloads indices for a specific category from ClassifiedEventBuffer.
 */
std::vector<int> downloadCategoryIndices(
    const ClassifiedEventBuffer& classifiedBuf,
    EventType type,
    const CUDAContext& context)
{
    std::size_t count = classifiedBuf.getCount(type);
    std::vector<int> h_indices(count);
    if (count > 0)
    {
        classifiedBuf.getIndexBuffer(type).copyToHostAsync(h_indices.data(), count, context.getStream());
        context.synchronize();
    }
    return h_indices;
}

/**
 * @brief Downloads eventTypes array from DecodedEventBuffer to host.
 */
std::vector<uint8_t> downloadEventTypes(
    const DecodedEventBuffer& decodedBuf,
    const CUDAContext& context)
{
    std::size_t n = decodedBuf.size();
    std::vector<uint8_t> h_types(n);
    if (n > 0)
    {
        decodedBuf.eventTypesBuffer().copyToHostAsync(h_types.data(), n, context.getStream());
        context.synchronize();
    }
    return h_types;
}

/**
 * @brief Test 1: ClassifiedEventBuffer default construction and capacity allocation.
 */
bool testBufferConstruction()
{
    ClassifiedEventBuffer emptyBuf;
    if (!emptyBuf.empty() || emptyBuf.size() != 0 || emptyBuf.capacity() != 0 || emptyBuf.isValid())
    {
        std::cerr << "  Failure: Default constructor invalid state\n";
        return false;
    }

    constexpr std::size_t cap = 2048;
    ClassifiedEventBuffer capBuf(cap);
    if (!capBuf.empty() || capBuf.size() != 0 || capBuf.capacity() != cap || !capBuf.isValid())
    {
        std::cerr << "  Failure: Capacity constructor invalid state\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 2: Category counts accuracy and index correctness for uniform distribution.
 */
bool testClassificationCorrectness()
{
    CUDAContext context;
    constexpr std::size_t N = 1400; // 200 events per category
    std::vector<double> uniformWeights = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    std::vector<Event> h_events = generateEventsWithDistribution(N, uniformWeights);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer decodedBuf(N);
    decodeEvents(replayBuf, decodedBuf, context);

    ClassifiedEventBuffer classifiedBuf(N);
    classifyEvents(decodedBuf, classifiedBuf, context);

    if (classifiedBuf.size() != N)
    {
        std::cerr << "  Failure: Classified total size = " << classifiedBuf.size() << " (expected " << N << ")\n";
        return false;
    }

    // Compute expected CPU counts and verify
    std::array<std::size_t, 7> cpuCounts = {0};
    for (const auto& ev : h_events)
    {
        cpuCounts[static_cast<std::size_t>(ev.type)]++;
    }

    const std::vector<EventType> allTypes = {
        EventType::Add, EventType::Cancel, EventType::Modify, EventType::Delete,
        EventType::ExecuteVisible, EventType::ExecuteHidden, EventType::TradingHalt
    };

    for (auto type : allTypes)
    {
        std::size_t expectedCount = cpuCounts[static_cast<std::size_t>(type)];
        std::size_t actualCount = classifiedBuf.getCount(type);
        if (actualCount != expectedCount)
        {
            std::cerr << "  Failure: Category " << static_cast<int>(type)
                      << " count = " << actualCount << " (expected " << expectedCount << ")\n";
            return false;
        }

        // Verify index correctness
        std::vector<int> h_indices = downloadCategoryIndices(classifiedBuf, type, context);
        if (h_indices.size() != expectedCount)
        {
            std::cerr << "  Failure: Downloaded index count mismatch for category " << static_cast<int>(type) << '\n';
            return false;
        }

        for (int idx : h_indices)
        {
            if (idx < 0 || static_cast<std::size_t>(idx) >= N)
            {
                std::cerr << "  Failure: Out-of-bounds index " << idx << " in category " << static_cast<int>(type) << '\n';
                return false;
            }
            if (h_events[idx].type != type)
            {
                std::cerr << "  Failure: Index " << idx << " points to event of type "
                          << static_cast<int>(h_events[idx].type) << " (expected " << static_cast<int>(type) << ")\n";
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Test 3: Empty input batch (0 events).
 */
bool testEmptyInput()
{
    CUDAContext context;
    DecodedEventBuffer emptyDecoded;
    ClassifiedEventBuffer classifiedBuf(100);

    classifyEvents(emptyDecoded, classifiedBuf, context);

    if (classifiedBuf.size() != 0 || !classifiedBuf.empty())
    {
        std::cerr << "  Failure: Empty input did not set classified size to 0\n";
        return false;
    }

    const std::vector<EventType> allTypes = {
        EventType::Add, EventType::Cancel, EventType::Modify, EventType::Delete,
        EventType::ExecuteVisible, EventType::ExecuteHidden, EventType::TradingHalt
    };

    for (auto type : allTypes)
    {
        if (classifiedBuf.getCount(type) != 0)
        {
            std::cerr << "  Failure: Category " << static_cast<int>(type) << " count non-zero for empty input\n";
            return false;
        }
    }

    return true;
}

/**
 * @brief Test 4: Stress test with skewed event distribution (e.g., 70% Add, 20% Cancel, etc.).
 */
bool testSkewedDistributionStressTest()
{
    CUDAContext context;
    constexpr std::size_t N = 10000;
    // 70% Add, 20% Cancel, 5% Modify, 2% Delete, 1% ExecuteVisible, 1% ExecuteHidden, 1% TradingHalt
    std::vector<double> skewedWeights = {70.0, 20.0, 5.0, 2.0, 1.0, 1.0, 1.0};
    std::vector<Event> h_events = generateEventsWithDistribution(N, skewedWeights, 999);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer decodedBuf(N);
    decodeEvents(replayBuf, decodedBuf, context);

    ClassifiedEventBuffer classifiedBuf;
    classifyEvents(decodedBuf, classifiedBuf, context); // Auto-reserves capacity

    if (classifiedBuf.size() != N)
    {
        std::cerr << "  Failure: Skewed stress test size mismatch (" << classifiedBuf.size() << " vs " << N << ")\n";
        return false;
    }

    std::array<std::size_t, 7> cpuCounts = {0};
    for (const auto& ev : h_events)
    {
        cpuCounts[static_cast<std::size_t>(ev.type)]++;
    }

    const std::vector<EventType> allTypes = {
        EventType::Add, EventType::Cancel, EventType::Modify, EventType::Delete,
        EventType::ExecuteVisible, EventType::ExecuteHidden, EventType::TradingHalt
    };

    std::unordered_set<int> allIndicesSeen;

    for (auto type : allTypes)
    {
        std::size_t expCount = cpuCounts[static_cast<std::size_t>(type)];
        std::size_t actCount = classifiedBuf.getCount(type);

        if (actCount != expCount)
        {
            std::cerr << "  Failure: Skewed count mismatch for category " << static_cast<int>(type)
                      << ": got " << actCount << ", expected " << expCount << '\n';
            return false;
        }

        std::vector<int> h_indices = downloadCategoryIndices(classifiedBuf, type, context);
        if (h_indices.size() != expCount)
        {
            std::cerr << "  Failure: Downloaded index size mismatch for category " << static_cast<int>(type) << '\n';
            return false;
        }

        for (int idx : h_indices)
        {
            if (h_events[idx].type != type)
            {
                std::cerr << "  Failure: Skewed index mismatch at index " << idx << '\n';
                return false;
            }
            allIndicesSeen.insert(idx);
        }
    }

    // Verify all N indices were uniquely classified across categories
    if (allIndicesSeen.size() != N)
    {
        std::cerr << "  Failure: Total unique indices seen = " << allIndicesSeen.size() << " (expected " << N << ")\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 5: Large batch classification (100,000 events).
 */
bool testLargeBatchClassification()
{
    CUDAContext context;
    constexpr std::size_t N = 100000;
    std::vector<double> weights = {50.0, 30.0, 10.0, 5.0, 2.0, 2.0, 1.0};
    std::vector<Event> h_events = generateEventsWithDistribution(N, weights, 777);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer decodedBuf(N);
    decodeEvents(replayBuf, decodedBuf, context);

    ClassifiedEventBuffer classifiedBuf;
    classifyEvents(decodedBuf, classifiedBuf, context);

    if (classifiedBuf.size() != N)
    {
        std::cerr << "  Failure: Large batch classification size mismatch\n";
        return false;
    }

    std::array<std::size_t, 7> cpuCounts = {0};
    for (const auto& ev : h_events)
    {
        cpuCounts[static_cast<std::size_t>(ev.type)]++;
    }

    const std::vector<EventType> allTypes = {
        EventType::Add, EventType::Cancel, EventType::Modify, EventType::Delete,
        EventType::ExecuteVisible, EventType::ExecuteHidden, EventType::TradingHalt
    };

    for (auto type : allTypes)
    {
        if (classifiedBuf.getCount(type) != cpuCounts[static_cast<std::size_t>(type)])
        {
            std::cerr << "  Failure: Large batch count mismatch for category " << static_cast<int>(type) << '\n';
            return false;
        }
    }

    return true;
}

/**
 * @brief Test 6: Move constructor and move assignment for ClassifiedEventBuffer.
 */
bool testMoveSemantics()
{
    CUDAContext context;
    constexpr std::size_t N = 1000;
    std::vector<double> weights = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
    std::vector<Event> h_events = generateEventsWithDistribution(N, weights);

    ReplayBuffer replayBuf(N);
    replayBuf.uploadBatch(h_events.data(), N, context.getStream());

    DecodedEventBuffer decodedBuf(N);
    decodeEvents(replayBuf, decodedBuf, context);

    ClassifiedEventBuffer srcBuf;
    classifyEvents(decodedBuf, srcBuf, context);

    // Move constructor
    ClassifiedEventBuffer dstBuf(std::move(srcBuf));
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
    ClassifiedEventBuffer dstBuf2;
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
        ClassifiedEventBuffer buf(0);
    }
    catch (const std::invalid_argument&)
    {
        ctorThrew = true;
    }
    if (!ctorThrew)
    {
        std::cerr << "  Failure: ClassifiedEventBuffer(0) did not throw std::invalid_argument\n";
        return false;
    }

    CUDAContext context;
    DecodedEventBuffer unallocatedDecoded;
    ClassifiedEventBuffer outputBuf(100);

    bool unallocatedThrew = false;
    try
    {
        unallocatedDecoded.allocate(100);
        unallocatedDecoded.release();
        classifyEvents(unallocatedDecoded, outputBuf, context);
    }
    catch (const std::invalid_argument&)
    {
        unallocatedThrew = true;
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
        {"2. Classification Correctness (Counts & Indices)", testClassificationCorrectness},
        {"3. Empty Input Batch (0 Events)", testEmptyInput},
        {"4. Skewed Distribution Stress Test (10K Events)", testSkewedDistributionStressTest},
        {"5. Large Batch Classification (100K Events)", testLargeBatchClassification},
        {"6. Move Semantics (Constructor & Assignment)", testMoveSemantics},
        {"7. Invalid Arguments Exception Handling", testInvalidArguments}
    };

    std::cout << "========================================\n";
    std::cout << "    EventClassifier Correctness Tests   \n";
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
