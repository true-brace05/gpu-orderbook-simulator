#pragma once

#include <map>
#include <list>
#include <unordered_map>

#include "Order.h"

class OrderBook
{
private:

    // Price -> FIFO queue of orders
    std::map<double, std::list<Order>> buyBook;
    std::map<double, std::list<Order>> sellBook;

    // Order ID -> iterator (for O(1) cancellation)
    std::unordered_map<int, std::list<Order>::iterator> orderIndex;

private:

    void matchBuyOrder(Order& order);

    void matchSellOrder(Order& order);

    void executeTrade(Order& incomingOrder,
                      Order& restingOrder);

    void removeEmptyPriceLevels();

public:

    void addOrder(const Order& order);

    void cancelOrder(int orderId);

    void printBook() const;
};