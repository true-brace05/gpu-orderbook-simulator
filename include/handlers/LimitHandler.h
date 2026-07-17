#pragma once

#include "Order.h"

class OrderBook;

class LimitHandler
{
public:
    void process(OrderBook& book, const Order& order);
};