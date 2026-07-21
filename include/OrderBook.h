#pragma once

#include <functional>
#include <list>
#include <map>
#include <unordered_map>
#include <optional>
#include <vector>
    #include <cstddef>    
#include "Order.h"
#include "dispatcher/OrderDispatcher.h"
#include "execution/ITradeListener.h"
#include "marketdata/OrderBookSnapshot.h"

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
    OrderDispatcher dispatcher;

private:
    // Highest price first
    std::map<double, std::list<Order>, std::greater<double>> buyBook;

    // Lowest price first
    std::map<double, std::list<Order>> sellBook;

    // Order ID -> Location
    std::unordered_map<int, OrderLocation> orderIndex;

    bool verbose = true;

    std::vector<ITradeListener*> tradeListeners;

private:
    void matchBuyOrder(Order& order);
    void matchSellOrder(Order& order);

    void addBuyOrder(const Order& order);
void addSellOrder(const Order& order);
bool replenishRestingIceberg(std::list<Order>& ordersAtPrice);
void notifyTradeListeners(const Trade& trade);
public:
    OrderBook() = default;

    void addOrder(const Order& order);

    bool cancelOrder(int orderId);

    // NEW: Reduce an order's remaining quantity.
    // If quantity becomes zero or negative,
    // the order is removed from the book.
    bool reduceOrderQuantity(
        int orderId,
        int quantity);

    void printBook() const;

    bool isEmpty() const;

    std::size_t getTotalOrders() const;

    bool hasOrder(int orderId) const;

    std::optional<Order> getOrder(int orderId) const;

    void setVerbose(bool enabled);

    void processLimitOrder(Order order);

    void processMarketOrder(Order order);

void addTradeListener(ITradeListener* listener);

double getBestBidPrice() const;

int getBestBidQuantity() const;

double getBestAskPrice() const;

int getBestAskQuantity() const;

OrderBookSnapshot getSnapshot(std::size_t levels = 10) const;
    };