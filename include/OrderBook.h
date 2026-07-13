#pragma once

#include <functional>
#include <list>
#include <map>
#include <unordered_map>

#include "Order.h"

/*
-----------------------------------------
OrderLocation

Stores everything required to locate an
order in O(1) time for cancellation.
-----------------------------------------
*/
struct OrderLocation
{
    Side side;
    double price;
    std::list<Order>::iterator iterator;
};

/*
-----------------------------------------
OrderBook

Core matching engine implementing
price-time priority.
-----------------------------------------
*/
class OrderBook
{
private:

    // Highest price first
    std::map<double, std::list<Order>, std::greater<double>> buyBook;

    // Lowest price first
    std::map<double, std::list<Order>> sellBook;

    // Order ID -> Location
    std::unordered_map<int, OrderLocation> orderIndex;

private:

    void matchBuyOrder(Order& order);
    void matchSellOrder(Order& order);

public:

    OrderBook() = default;

    void addOrder(const Order& order);

    bool cancelOrder(int orderId);

    void printBook() const;
};