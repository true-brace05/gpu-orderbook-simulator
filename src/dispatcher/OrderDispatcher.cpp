#include "dispatcher/OrderDispatcher.h"
#include "OrderBook.h"

void OrderDispatcher::dispatch(OrderBook& book, const Order& order)
{
    switch (order.type)
    {
        case OrderType::Limit:
            limitHandler.process(book, order);
            break;

        case OrderType::Market:
            marketHandler.process(book, order);
            break;

        case OrderType::Iceberg:
            limitHandler.process(book, order);
            break;
    }
}
