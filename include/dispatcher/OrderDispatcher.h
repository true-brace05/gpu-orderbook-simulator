#pragma once

#include "Order.h"
#include "handlers/LimitHandler.h"
#include "handlers/MarketHandler.h"

class OrderBook;

class OrderDispatcher
{
private:
    LimitHandler limitHandler;
    MarketHandler marketHandler;

public:
    OrderDispatcher() = default;

    void dispatch(OrderBook& book, const Order& order);
};