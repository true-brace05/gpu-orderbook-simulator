#pragma once

#include "Order.h"

class OrderBook;

class MarketHandler
{
public:
    void process(OrderBook& book, const Order& order);
};