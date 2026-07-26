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
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

constexpr double tickSize = 0.01;

/**
 * @brief Helper to create a single synthetic Add event.
 */
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

/**
 * @brief Helper to upload events, decode, classify, and build resting PriceLevelBuffer.
 */
void buildRestingBookFromEvents(
    const std::vector<Event>& events,
    DecodedEventBuffer& decodedBuf,
    ClassifiedEventBuffer& classifiedBuf,
    PriceLevelBuffer& levelBuf,
    CUDAContext& context)
{
    const std::size_t n = events.size();
    if (n == 0)
    {
        levelBuf.clear();
        return;
    }

    ReplayBuffer replayBuf(n);
    replayBuf.uploadBatch(events.data(), n, context.getStream());

    decodedBuf.allocate(n);
    decodeEvents(replayBuf, decodedBuf, context);

    classifiedBuf.allocate(n);
    classifyEvents(decodedBuf, classifiedBuf, context);

    levelBuf.allocate(n);
    buildPriceLevels(decodedBuf, classifiedBuf, levelBuf, context, tickSize);
}

/**
 * @brief Test 1: TradeBuffer default & capacity construction and move semantics.
 */
bool testTradeBufferConstructionAndMove()
{
    TradeBuffer emptyBuf;
    if (!emptyBuf.empty() || emptyBuf.size() != 0 || emptyBuf.capacity() != 0 || emptyBuf.isValid())
    {
        std::cerr << "  Failure: Default TradeBuffer invalid state\n";
        return false;
    }

    constexpr std::size_t cap = 512;
    TradeBuffer capBuf(cap);
    if (!capBuf.empty() || capBuf.size() != 0 || capBuf.capacity() != cap || !capBuf.isValid())
    {
        std::cerr << "  Failure: Capacity TradeBuffer invalid state\n";
        return false;
    }

    // Move constructor
    TradeBuffer dstBuf(std::move(capBuf));
    if (dstBuf.capacity() != cap || !dstBuf.isValid() || capBuf.capacity() != 0 || capBuf.isValid())
    {
        std::cerr << "  Failure: Move constructor failed to transfer state\n";
        return false;
    }

    // Move assignment
    TradeBuffer dstBuf2;
    dstBuf2 = std::move(dstBuf);
    if (dstBuf2.capacity() != cap || !dstBuf2.isValid() || dstBuf.capacity() != 0 || dstBuf.isValid())
    {
        std::cerr << "  Failure: Move assignment failed to transfer state\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 2: No match when incoming order price does not cross book.
 */
bool testNoMatch()
{
    CUDAContext context;

    // Resting Sell at $105.00
    std::vector<Event> restingEvents = { createAddEvent(1, Side::Sell, 105.00, 100, 1000) };
    DecodedEventBuffer restDecoded;
    ClassifiedEventBuffer restClassified;
    PriceLevelBuffer restingBook;
    buildRestingBookFromEvents(restingEvents, restDecoded, restClassified, restingBook, context);

    // Incoming Buy at $100.00 (below $105.00 -> no match)
    std::vector<Event> incEvents = { createAddEvent(2, Side::Buy, 100.00, 50, 1001) };
    ReplayBuffer incReplay(1);
    incReplay.uploadBatch(incEvents.data(), 1, context.getStream());

    DecodedEventBuffer incDecoded(1);
    decodeEvents(incReplay, incDecoded, context);

    ClassifiedEventBuffer incClassified(1);
    classifyEvents(incDecoded, incClassified, context);

    TradeBuffer tradeBuf(10);
    MatchingStatistics stats;

    matchAddOrders(incDecoded, incClassified, restDecoded, restingBook, tradeBuf, stats, context, tickSize);

    if (tradeBuf.size() != 0 || stats.tradesGenerated != 0 || stats.restedOrders != 1)
    {
        std::cerr << "  Failure: Expected 0 trades and 1 rested order, got trades = "
                  << tradeBuf.size() << ", rested = " << stats.restedOrders << '\n';
        return false;
    }

    return true;
}

/**
 * @brief Test 3: Full fill match.
 */
bool testFullFill()
{
    CUDAContext context;

    // Resting Sell at $100.00 (100 qty)
    std::vector<Event> restingEvents = { createAddEvent(10, Side::Sell, 100.00, 100, 1000) };
    DecodedEventBuffer restDecoded;
    ClassifiedEventBuffer restClassified;
    PriceLevelBuffer restingBook;
    buildRestingBookFromEvents(restingEvents, restDecoded, restClassified, restingBook, context);

    // Incoming Buy at $100.00 (100 qty -> exact full fill)
    std::vector<Event> incEvents = { createAddEvent(20, Side::Buy, 100.00, 100, 1001) };
    ReplayBuffer incReplay(1);
    incReplay.uploadBatch(incEvents.data(), 1, context.getStream());

    DecodedEventBuffer incDecoded(1);
    decodeEvents(incReplay, incDecoded, context);

    ClassifiedEventBuffer incClassified(1);
    classifyEvents(incDecoded, incClassified, context);

    TradeBuffer tradeBuf(10);
    MatchingStatistics stats;

    matchAddOrders(incDecoded, incClassified, restDecoded, restingBook, tradeBuf, stats, context, tickSize);

    if (tradeBuf.size() != 1 || stats.tradesGenerated != 1 || stats.fullyMatchedOrders != 1)
    {
        std::cerr << "  Failure: Full fill stats mismatch (trades: " << tradeBuf.size()
                  << ", fullyMatched: " << stats.fullyMatchedOrders << ")\n";
        return false;
    }

    std::vector<TradeRecord> h_trades(1);
    tradeBuf.getTradesBuffer().copyToHostAsync(h_trades.data(), 1, context.getStream());
    context.synchronize();

    const auto& trade = h_trades[0];
    if (trade.buyOrderId != 20 || trade.sellOrderId != 10 || trade.executedQuantity != 100 || trade.aggressorSide != 0)
    {
        std::cerr << "  Failure: TradeRecord attributes mismatch in full fill\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 4: Partial fill match.
 */
bool testPartialFill()
{
    CUDAContext context;

    // Resting Sell at $100.00 (50 qty)
    std::vector<Event> restingEvents = { createAddEvent(100, Side::Sell, 100.00, 50, 2000) };
    DecodedEventBuffer restDecoded;
    ClassifiedEventBuffer restClassified;
    PriceLevelBuffer restingBook;
    buildRestingBookFromEvents(restingEvents, restDecoded, restClassified, restingBook, context);

    // Incoming Buy at $100.00 (150 qty -> partial fill 50, residual 100)
    std::vector<Event> incEvents = { createAddEvent(200, Side::Buy, 100.00, 150, 2001) };
    ReplayBuffer incReplay(1);
    incReplay.uploadBatch(incEvents.data(), 1, context.getStream());

    DecodedEventBuffer incDecoded(1);
    decodeEvents(incReplay, incDecoded, context);

    ClassifiedEventBuffer incClassified(1);
    classifyEvents(incDecoded, incClassified, context);

    TradeBuffer tradeBuf(10);
    MatchingStatistics stats;

    matchAddOrders(incDecoded, incClassified, restDecoded, restingBook, tradeBuf, stats, context, tickSize);

    if (tradeBuf.size() != 1 || stats.partiallyMatchedOrders != 1)
    {
        std::cerr << "  Failure: Partial fill stats mismatch (trades: " << tradeBuf.size()
                  << ", partiallyMatched: " << stats.partiallyMatchedOrders << ")\n";
        return false;
    }

    std::vector<TradeRecord> h_trades(1);
    tradeBuf.getTradesBuffer().copyToHostAsync(h_trades.data(), 1, context.getStream());
    context.synchronize();

    if (h_trades[0].executedQuantity != 50)
    {
        std::cerr << "  Failure: Partial fill executed quantity = " << h_trades[0].executedQuantity << " (expected 50)\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 5: Multiple price levels (incoming order sweeps book).
 */
bool testMultiplePriceLevels()
{
    CUDAContext context;

    // Resting Sells: 100 at $100.00, 200 at $100.50
    std::vector<Event> restingEvents = {
        createAddEvent(1, Side::Sell, 100.00, 100, 1000),
        createAddEvent(2, Side::Sell, 100.50, 200, 1001)
    };
    DecodedEventBuffer restDecoded;
    ClassifiedEventBuffer restClassified;
    PriceLevelBuffer restingBook;
    buildRestingBookFromEvents(restingEvents, restDecoded, restClassified, restingBook, context);

    // Incoming Buy at $101.00 (300 qty -> sweeps $100.00 and $100.50)
    std::vector<Event> incEvents = { createAddEvent(99, Side::Buy, 101.00, 300, 2000) };
    ReplayBuffer incReplay(1);
    incReplay.uploadBatch(incEvents.data(), 1, context.getStream());

    DecodedEventBuffer incDecoded(1);
    decodeEvents(incReplay, incDecoded, context);

    ClassifiedEventBuffer incClassified(1);
    classifyEvents(incDecoded, incClassified, context);

    TradeBuffer tradeBuf(10);
    MatchingStatistics stats;

    matchAddOrders(incDecoded, incClassified, restDecoded, restingBook, tradeBuf, stats, context, tickSize);

    if (tradeBuf.size() != 2 || stats.fullyMatchedOrders != 1)
    {
        std::cerr << "  Failure: Multiple price level sweep expected 2 trades, got " << tradeBuf.size() << '\n';
        return false;
    }

    std::vector<TradeRecord> h_trades(2);
    tradeBuf.getTradesBuffer().copyToHostAsync(h_trades.data(), 2, context.getStream());
    context.synchronize();

    uint32_t tick100 = static_cast<uint32_t>(std::llround(100.00 / tickSize));
    uint32_t tick10050 = static_cast<uint32_t>(std::llround(100.50 / tickSize));

    if (h_trades[0].priceTick != tick100 || h_trades[0].executedQuantity != 100)
    {
        std::cerr << "  Failure: First swept trade level mismatch\n";
        return false;
    }

    if (h_trades[1].priceTick != tick10050 || h_trades[1].executedQuantity != 200)
    {
        std::cerr << "  Failure: Second swept trade level mismatch\n";
        return false;
    }

    return true;
}

/**
 * @brief Test 6: FIFO correctness inside same price level.
 */
bool testFIFOCorrectness()
{
    CUDAContext context;

    // Resting Sells at $100.00: Order 1 (100 qty, t=1000), Order 2 (100 qty, t=1001)
    std::vector<Event> restingEvents = {
        createAddEvent(1, Side::Sell, 100.00, 100, 1000),
        createAddEvent(2, Side::Sell, 100.00, 100, 1001)
    };
    DecodedEventBuffer restDecoded;
    ClassifiedEventBuffer restClassified;
    PriceLevelBuffer restingBook;
    buildRestingBookFromEvents(restingEvents, restDecoded, restClassified, restingBook, context);

    // Incoming Buy at $100.00 (100 qty -> must match Order 1 first)
    std::vector<Event> incEvents = { createAddEvent(55, Side::Buy, 100.00, 100, 2000) };
    ReplayBuffer incReplay(1);
    incReplay.uploadBatch(incEvents.data(), 1, context.getStream());

    DecodedEventBuffer incDecoded(1);
    decodeEvents(incReplay, incDecoded, context);

    ClassifiedEventBuffer incClassified(1);
    classifyEvents(incDecoded, incClassified, context);

    TradeBuffer tradeBuf(10);
    MatchingStatistics stats;

    matchAddOrders(incDecoded, incClassified, restDecoded, restingBook, tradeBuf, stats, context, tickSize);

    std::vector<TradeRecord> h_trades(1);
    tradeBuf.getTradesBuffer().copyToHostAsync(h_trades.data(), 1, context.getStream());
    context.synchronize();

    if (h_trades[0].sellOrderId != 1)
    {
        std::cerr << "  Failure: FIFO violated! Matched sell order ID " << h_trades[0].sellOrderId << " (expected 1)\n";
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
    DecodedEventBuffer incDecoded;
    ClassifiedEventBuffer incClassified;
    PriceLevelBuffer restingBook;
    TradeBuffer tradeBuf(10);
    MatchingStatistics stats;

    bool tickSizeThrew = false;
    try
    {
        matchAddOrders(incDecoded, incClassified, incDecoded, restingBook, tradeBuf, stats, context, -0.01);
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
        {"1. TradeBuffer Construction & Move Semantics", testTradeBufferConstructionAndMove},
        {"2. No Match Execution", testNoMatch},
        {"3. Full Fill Execution", testFullFill},
        {"4. Partial Fill Execution", testPartialFill},
        {"5. Multiple Price Levels Sweep", testMultiplePriceLevels},
        {"6. FIFO Matching Priority Inside Price Level", testFIFOCorrectness},
        {"7. Invalid Arguments Exception Handling", testInvalidArguments}
    };

    std::cout << "========================================\n";
    std::cout << "   GPUMatchingEngine Correctness Tests  \n";
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
