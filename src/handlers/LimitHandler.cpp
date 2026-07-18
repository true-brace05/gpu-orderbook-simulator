#include "handlers/LimitHandler.h"
#include "OrderBook.h"

void LimitHandler::process(OrderBook& book, const Order& order)
{
    book.processLimitOrder(order);
}