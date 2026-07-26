#include "cuda/GPUOrderBookStateManager.h"
#include "cuda/ClassifiedEventBuffer.h"
#include "cuda/DecodedEventBuffer.h"
#include "cuda/EventClassifier.h"
#include "cuda/EventDecoder.h"
#include "cuda/ReplayBuffer.h"
#include "cuda/CUDAContext.h"
#include "replay/Event.h"
#include "Types.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{
Event createAddEvent(int orderId, Side side, double price, int quantity, uint64_t timestamp)
{
    Event ev;
    ev.timestamp = timestamp;
    ev.type = EventType::Add;
    ev.orderId = orderId;
    ev.order.id = orderId;
    ev.order.side = side;
    ev.order.type = OrderType::Limit;
    ev.order.price = price;
    ev.order.quantity = quantity;
    ev.order.timestamp = timestamp;
    ev.order.displayQuantity = quantity;
    ev.order.reserveQuantity = 0;
    return ev;
}

Event createCancelEvent(int orderId, Side side, double price, int cancelQuantity, uint64_t timestamp)
{
    Event ev;
    ev.timestamp = timestamp;
    ev.type = EventType::Cancel;
    ev.orderId = orderId;
    ev.order.id = orderId;
    ev.order.side = side;
    ev.order.type = OrderType::Limit;
    ev.order.price = price;
    ev.order.quantity = cancelQuantity;
    ev.order.timestamp = timestamp;
    ev.order.displayQuantity = cancelQuantity;
    ev.order.reserveQuantity = 0;
    return ev;
}
} // namespace

void testAddSingleOrder()
{
    std::cout << "[Test] Add Single Order ... ";

    CUDAContext context;
    std::vector<Event> events = {
        createAddEvent(101, Side::Buy, 100.0, 50, 1000)
    };

    ReplayBuffer replay(100);
    replay.uploadBatch(events.data(), events.size(), context.getStream());

    DecodedEventBuffer decoded(100);
    decodeEventsAsync(replay, decoded, context.getStream());

    ClassifiedEventBuffer classified(100);
    classifyEventsAsync(decoded, classified, context.getStream());

    GPUOrderBookStateManager stateMgr(1000);
    stateMgr.processEventsShadow(classified, decoded, 0.01, context.getStream());
    context.synchronize();

    std::size_t activeCount = stateMgr.getActiveOrderCount(context.getStream());
    assert(activeCount == 1);

    std::string errMsg;
    bool ok = stateMgr.verifyBatch(decoded, 0.01, &errMsg);
    if (!ok)
    {
        std::cerr << "Verification failed: " << errMsg << "\n";
    }
    assert(ok);

    std::cout << "PASSED\n";
}

void testAddMultipleOrders()
{
    std::cout << "[Test] Add Multiple Orders ... ";

    CUDAContext context;
    const std::size_t N = 1000;
    std::vector<Event> events;
    events.reserve(N);

    for (std::size_t i = 0; i < N; ++i)
    {
        int orderId = static_cast<int>(i + 1);
        double price = 100.0 + (i % 20) * 0.5;
        events.push_back(createAddEvent(orderId, Side::Buy, price, 10, static_cast<uint64_t>(1000 + i)));
    }

    ReplayBuffer replay(N);
    replay.uploadBatch(events.data(), events.size(), context.getStream());

    DecodedEventBuffer decoded(N);
    decodeEventsAsync(replay, decoded, context.getStream());

    ClassifiedEventBuffer classified(N);
    classifyEventsAsync(decoded, classified, context.getStream());

    GPUOrderBookStateManager stateMgr(N * 2);
    stateMgr.processEventsShadow(classified, decoded, 0.01, context.getStream());
    context.synchronize();

    std::size_t activeCount = stateMgr.getActiveOrderCount(context.getStream());
    assert(activeCount == N);

    std::string errMsg;
    bool ok = stateMgr.verifyBatch(decoded, 0.01, &errMsg);
    if (!ok)
    {
        std::cerr << "Verification failed: " << errMsg << "\n";
    }
    assert(ok);

    std::cout << "PASSED\n";
}

void testCancelFullOrder()
{
    std::cout << "[Test] Cancel Full Order ... ";

    CUDAContext context;
    std::vector<Event> events = {
        createAddEvent(101, Side::Buy, 100.0, 50, 1000),
        createCancelEvent(101, Side::Buy, 100.0, 50, 1001)
    };

    ReplayBuffer replay(100);
    replay.uploadBatch(events.data(), events.size(), context.getStream());

    DecodedEventBuffer decoded(100);
    decodeEventsAsync(replay, decoded, context.getStream());

    ClassifiedEventBuffer classified(100);
    classifyEventsAsync(decoded, classified, context.getStream());

    GPUOrderBookStateManager stateMgr(1000);
    stateMgr.processEventsShadow(classified, decoded, 0.01, context.getStream());
    context.synchronize();

    std::string errMsg;
    bool ok = stateMgr.verifyBatch(decoded, 0.01, &errMsg);
    if (!ok)
    {
        std::cerr << "Verification failed: " << errMsg << "\n";
    }
    assert(ok);

    std::cout << "PASSED\n";
}

void testCancelPartialOrder()
{
    std::cout << "[Test] Cancel Partial Order ... ";

    CUDAContext context;
    std::vector<Event> events = {
        createAddEvent(201, Side::Sell, 105.0, 100, 1000),
        createCancelEvent(201, Side::Sell, 105.0, 40, 1001)
    };

    ReplayBuffer replay(100);
    replay.uploadBatch(events.data(), events.size(), context.getStream());

    DecodedEventBuffer decoded(100);
    decodeEventsAsync(replay, decoded, context.getStream());

    ClassifiedEventBuffer classified(100);
    classifyEventsAsync(decoded, classified, context.getStream());

    GPUOrderBookStateManager stateMgr(1000);
    stateMgr.processEventsShadow(classified, decoded, 0.01, context.getStream());
    context.synchronize();

    std::string errMsg;
    bool ok = stateMgr.verifyBatch(decoded, 0.01, &errMsg);
    if (!ok)
    {
        std::cerr << "Verification failed: " << errMsg << "\n";
    }
    assert(ok);

    std::cout << "PASSED\n";
}

void testCancelNonExistentOrder()
{
    std::cout << "[Test] Cancel Non-Existent Order ... ";

    CUDAContext context;
    std::vector<Event> events = {
        createCancelEvent(999, Side::Buy, 100.0, 10, 1000)
    };

    ReplayBuffer replay(100);
    replay.uploadBatch(events.data(), events.size(), context.getStream());

    DecodedEventBuffer decoded(100);
    decodeEventsAsync(replay, decoded, context.getStream());

    ClassifiedEventBuffer classified(100);
    classifyEventsAsync(decoded, classified, context.getStream());

    GPUOrderBookStateManager stateMgr(1000);
    stateMgr.processEventsShadow(classified, decoded, 0.01, context.getStream());
    context.synchronize();

    std::string errMsg;
    bool ok = stateMgr.verifyBatch(decoded, 0.01, &errMsg);
    if (!ok)
    {
        std::cerr << "Verification failed: " << errMsg << "\n";
    }
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
