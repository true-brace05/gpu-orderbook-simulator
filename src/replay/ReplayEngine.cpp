#include "replay/ReplayEngine.h"
#include "strategy/IStrategy.h"

ReplayEngine::ReplayEngine(OrderBook& book)
    : orderBook(book)
{
}

void ReplayEngine::process(const Event& event)
{
    switch (event.type)
    {
        case EventType::Add:
            orderBook.addOrder(event.order);
            break;

        case EventType::Cancel:
            orderBook.cancelOrder(event.orderId);
            break;

        case EventType::Modify:
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
    while (reader.hasNext())
{
    Event event = reader.next();

    for (IEventListener* listener : eventListeners)
    {
        listener->onEvent(event);
    }

    process(event);
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