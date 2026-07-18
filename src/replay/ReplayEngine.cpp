#include "replay/ReplayEngine.h"

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
}

void ReplayEngine::replay(IEventReader& reader)
{
    while (reader.hasNext())
    {
        process(reader.next());
    }
}