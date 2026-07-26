#include "cuda/GPUOrderBookStateManager.h"
#include "cuda/ClassifiedEventBuffer.h"
#include "cuda/DecodedEventBuffer.h"
#include "cuda/EventClassifier.h"
#include "cuda/ReplayBuffer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

void testAddSingleOrder()
{
    std::cout << "[Test] Add Single Order ... ";

    ReplayBuffer replay(100);
    std::vector<Event> events = {
        {1, EventType::Add, {101, Side::Buy, OrderType::Limit, 100.0, 50, 1000}, -1}
    };
    replay.uploadEvents(events);

    DecodedEventBuffer decoded(100);
    decodeEventsAsync(replay, decoded);

    ClassifiedEventBuffer classified(100);
    classifyEventsAsync(decoded, classified);

    GPUOrderBookStateManager stateMgr(1000);
    stateMgr.processEventsShadow(classified, decoded, 0.01);

    std::size_t activeCount = stateMgr.getActiveOrderCount();
    assert(activeCount == 1);

    std::string errMsg;
    bool ok = stateMgr.verifyBatch(decoded, 0.01, &errMsg);
    assert(ok);

    std::cout << "PASSED\n";
}

void testAddMultipleOrders()
{
    std::cout << "[Test] Add Multiple Orders ... ";

    const std::size_t N = 1000;
    ReplayBuffer replay(N);
    std::vector<Event> events;
    events.reserve(N);

    for (std::size_t i = 0; i < N; ++i)
    {
        int orderId = static_cast<int>(i + 1);
        double price = 100.0 + (i % 20) * 0.5;
        events.push_back({static_cast<uint64_t>(i + 1), EventType::Add, {orderId, Side::Buy, OrderType::Limit, price, 10, static_cast<uint64_t>(1000 + i)}, -1});
    }
    replay.uploadEvents(events);

    DecodedEventBuffer decoded(N);
    decodeEventsAsync(replay, decoded);

    ClassifiedEventBuffer classified(N);
    classifyEventsAsync(decoded, classified);

    GPUOrderBookStateManager stateMgr(N * 2);
    stateMgr.processEventsShadow(classified, decoded, 0.01);

    std::size_t activeCount = stateMgr.getActiveOrderCount();
    assert(activeCount == N);

    std::string errMsg;
    bool ok = stateMgr.verifyBatch(decoded, 0.01, &errMsg);
    assert(ok);

    std::cout << "PASSED\n";
}

void testCancelFullOrder()
{
    std::cout << "[Test] Cancel Full Order ... ";

    ReplayBuffer replay(100);
    std::vector<Event> events = {
        {1, EventType::Add, {101, Side::Buy, OrderType::Limit, 100.0, 50, 1000}, 101},
        {2, EventType::Cancel, {101, Side::Buy, OrderType::Limit, 100.0, 50, 1001}, 101}
    };
    replay.uploadEvents(events);

    DecodedEventBuffer decoded(100);
    decodeEventsAsync(replay, decoded);

    ClassifiedEventBuffer classified(100);
    classifyEventsAsync(decoded, classified);

    GPUOrderBookStateManager stateMgr(1000);
    stateMgr.processEventsShadow(classified, decoded, 0.01);

    std::string errMsg;
    bool ok = stateMgr.verifyBatch(decoded, 0.01, &errMsg);
    assert(ok);

    std::cout << "PASSED\n";
}

void testCancelPartialOrder()
{
    std::cout << "[Test] Cancel Partial Order ... ";

    ReplayBuffer replay(100);
    std::vector<Event> events = {
        {1, EventType::Add, {201, Side::Sell, OrderType::Limit, 105.0, 100, 1000}, 201},
        {2, EventType::Cancel, {201, Side::Sell, OrderType::Limit, 105.0, 40, 1001}, 201}
    };
    replay.uploadEvents(events);

    DecodedEventBuffer decoded(100);
    decodeEventsAsync(replay, decoded);

    ClassifiedEventBuffer classified(100);
    classifyEventsAsync(decoded, classified);

    GPUOrderBookStateManager stateMgr(1000);
    stateMgr.processEventsShadow(classified, decoded, 0.01);

    std::string errMsg;
    bool ok = stateMgr.verifyBatch(decoded, 0.01, &errMsg);
    assert(ok);

    std::cout << "PASSED\n";
}

void testCancelNonExistentOrder()
{
    std::cout << "[Test] Cancel Non-Existent Order ... ";

    ReplayBuffer replay(100);
    std::vector<Event> events = {
        {1, EventType::Cancel, {999, Side::Buy, OrderType::Limit, 100.0, 10, 1000}, -1}
    };
    replay.uploadEvents(events);

    DecodedEventBuffer decoded(100);
    decodeEventsAsync(replay, decoded);

    ClassifiedEventBuffer classified(100);
    classifyEventsAsync(decoded, classified);

    GPUOrderBookStateManager stateMgr(1000);
    stateMgr.processEventsShadow(classified, decoded, 0.01);

    std::string errMsg;
    bool ok = stateMgr.verifyBatch(decoded, 0.01, &errMsg);
    assert(ok);

    std::cout << "PASSED\n";
}

int main()
{
    std::cout << "====================================================\n";
    std::cout << "  GPU Order Book State Manager Phase 1 Tests (Shadow Mode)\n";
    std::cout << "====================================================\n";

    testAddSingleOrder();
    testAddMultipleOrders();
    testCancelFullOrder();
    testCancelPartialOrder();
    testCancelNonExistentOrder();

    std::cout << "All GPUOrderBookStateManager tests PASSED successfully!\n";
    return 0;
}
