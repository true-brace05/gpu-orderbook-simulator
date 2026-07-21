#include "replay/ReplayEngine.h"
#include "strategy/IStrategy.h"
#include <limits>
#include <chrono>


ReplayEngine::ReplayEngine(OrderBook& book)
    : orderBook(book)
{
}

void ReplayEngine::process(const Event& event)
{
    switch (event.type)
{
    case EventType::Add:
    ++statistics.addEvents;
    orderBook.addOrder(event.order);
    break;

case EventType::Cancel:
    ++statistics.cancelEvents;
    orderBook.reduceOrderQuantity(
        event.orderId,
        event.order.quantity);
    break;

case EventType::Delete:
    ++statistics.deleteEvents;
    orderBook.cancelOrder(event.orderId);
    break;

case EventType::ExecuteVisible:
    ++statistics.visibleExecutions;
    orderBook.reduceOrderQuantity(
        event.orderId,
        event.order.quantity);
    break;

case EventType::ExecuteHidden:
    ++statistics.hiddenExecutions;
    // Hidden executions do not modify the visible order book.
    break;

case EventType::TradingHalt:
    ++statistics.tradingHalts;
    break;
}

    if (strategy)
    {
        std::vector<Order> generatedOrders =
            strategy->onEvent(event, orderBook);

        for (const Order& order : generatedOrders)
        {
            orderBook.addOrder(order);
        }
    }
}

void ReplayEngine::replay(IEventReader& reader)
{
    replay(reader, std::numeric_limits<std::size_t>::max());
}

void ReplayEngine::replay(
    IEventReader& reader,
    std::size_t maxEvents)
{
    auto start =
        std::chrono::high_resolution_clock::now();

    std::size_t processedEvents = 0;

    while (reader.hasNext() &&
           processedEvents < maxEvents)
    {
        Event event = reader.next();

        for (IEventListener* listener : eventListeners)
        {
            listener->onEvent(event);
        }

        process(event);

        ++statistics.processedEvents;
        ++processedEvents;
    }

    auto end =
        std::chrono::high_resolution_clock::now();

    statistics.replayTimeMs =
        std::chrono::duration<double, std::milli>(
            end - start).count();

    if (statistics.replayTimeMs > 0.0)
    {
        statistics.eventsPerSecond =
            statistics.processedEvents /
            (statistics.replayTimeMs / 1000.0);
    }
}

void ReplayEngine::setStrategy(IStrategy* newStrategy)
{
    strategy = newStrategy;
}

void ReplayEngine::addEventListener(IEventListener* listener)
{
    eventListeners.push_back(listener);
}

const ReplayStatistics& ReplayEngine::getStatistics() const
{
    return statistics;
}