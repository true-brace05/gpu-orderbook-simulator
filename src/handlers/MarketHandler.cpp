#include "handlers/MarketHandler.h"
#include "OrderBook.h"

void MarketHandler::process(OrderBook& book, const Order& order)
{
    book.processMarketOrder(order);
}