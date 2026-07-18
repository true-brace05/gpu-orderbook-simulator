#include "OrderBook.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

void OrderBook::addOrder(const Order& order)
{
// dispatcher.dispatch(*this, order);



    if (hasOrder(order.id))
{
    std::cerr << "Duplicate Order ID.\n";
    return;
}


    if (order.quantity <= 0)
    {
        std::cerr << "Invalid quantity.\n";
        return;
    }

    if (order.type == OrderType::Limit && order.price <= 0)
{
    std::cerr << "Invalid price.\n";
    return;
}
    dispatcher.dispatch(*this, order);

    

    
}

void OrderBook::matchBuyOrder(Order& order)
{
    while (order.quantity > 0 && !sellBook.empty())
    {
        auto bestSell = sellBook.begin();

        if (order.type == OrderType::Limit &&
    bestSell->first > order.price)
{
    break;
}

        Order& restingOrder = bestSell->second.front();
        const int tradeQuantity = std::min(order.quantity, restingOrder.quantity);

        order.quantity -= tradeQuantity;
        restingOrder.quantity -= tradeQuantity;

        if (verbose)
{
    std::cout << "TRADE -> "
              << tradeQuantity
              << " @ "
              << bestSell->first
              << '\n';
}

        if (restingOrder.quantity == 0)
{
    const int restingOrderId = restingOrder.id;

    orderIndex.erase(restingOrderId);

    bestSell->second.pop_front();
}

        if (bestSell->second.empty())
        {
            sellBook.erase(bestSell);
        }
    }
}

void OrderBook::matchSellOrder(Order& order)
{
    while (order.quantity > 0 && !buyBook.empty())
    {
        auto bestBuy = buyBook.begin();

        if (order.type == OrderType::Limit &&
    bestBuy->first < order.price)
{
    break;
}

        Order& restingOrder = bestBuy->second.front();
        const int tradeQuantity = std::min(order.quantity, restingOrder.quantity);

        order.quantity -= tradeQuantity;
        restingOrder.quantity -= tradeQuantity;

        if (verbose)
{
    std::cout << "TRADE -> "
              << tradeQuantity
              << " @ "
              << bestBuy->first
              << '\n';
}

        if (restingOrder.quantity == 0)
{
    const int restingOrderId = restingOrder.id;

    orderIndex.erase(restingOrderId);
    bestBuy->second.pop_front();
}

        if (bestBuy->second.empty())
        {
            buyBook.erase(bestBuy);
        }
    }
}

bool OrderBook::cancelOrder(const int orderId)
{
    const auto indexEntry = orderIndex.find(orderId);

    if (indexEntry == orderIndex.end())
    {
        return false;
    }

    const OrderLocation& location = indexEntry->second;

    if (location.side == Side::Buy)
    {
        auto priceLevel = buyBook.find(location.price);
        priceLevel->second.erase(location.iterator);

        if (priceLevel->second.empty())
        {
            buyBook.erase(priceLevel);
        }
    }
    else
    {
        auto priceLevel = sellBook.find(location.price);
        priceLevel->second.erase(location.iterator);

        if (priceLevel->second.empty())
        {
            sellBook.erase(priceLevel);
        }
    }

    orderIndex.erase(indexEntry);
    return true;
}

void OrderBook::printBook() const
{
    std::cout << "\n";
std::cout << "=====================================\n";
std::cout << "             BUY BOOK\n";
std::cout << "=====================================\n";
    std::cout << "Price\tOrder ID\tQuantity\tTimestamp\n";

    for (const auto& [price, orders] : buyBook)
    {
        for (const Order& order : orders)
        {
            std::cout
    << std::left
    << std::setw(10) << price
    << std::setw(10) << order.id
    << std::setw(10) << order.quantity
    << std::setw(10) << order.timestamp
    << '\n';
        }
    }

    std::cout << "\n";
std::cout << "=====================================\n";
std::cout << "             SELL BOOK\n";
std::cout << "=====================================\n";
    std::cout << "Price\tOrder ID\tQuantity\tTimestamp\n";

    for (const auto& [price, orders] : sellBook)
    {
        for (const Order& order : orders)
        {
            std::cout
    << std::left
    << std::setw(10) << price
    << std::setw(10) << order.id
    << std::setw(10) << order.quantity
    << std::setw(10) << order.timestamp
    << '\n';
        }
    }
}



bool OrderBook::isEmpty() const
{
    return buyBook.empty() && sellBook.empty();
}




std::size_t OrderBook::getTotalOrders() const
{
    std::size_t totalOrders = 0;

    for (const auto& [price, orders] : buyBook)
    {
        totalOrders += orders.size();
    }

    for (const auto& [price, orders] : sellBook)
    {
        totalOrders += orders.size();
    }

    return totalOrders;
}

bool OrderBook::hasOrder(int orderId) const
{
    return orderIndex.find(orderId) != orderIndex.end();
}


std::optional<Order> OrderBook::getOrder(int orderId) const
{
    auto it = orderIndex.find(orderId);

    if (it == orderIndex.end())
    {
        return std::nullopt;
    }

    return *(it->second.iterator);
}

void OrderBook::setVerbose(bool enabled)
{
    verbose = enabled;
}

void OrderBook::processLimitOrder( Order order)
{
    if (order.side == Side::Buy)
    {
        matchBuyOrder(order);

        if (order.quantity > 0)
        {
           addBuyOrder(order);
        }
    }
    else
    {
        matchSellOrder(order);

        if (order.quantity > 0)
        {
            addSellOrder(order);
        }
    }
}

void OrderBook::processMarketOrder(Order order)
{
    
    if (order.side == Side::Buy)
    {
        matchBuyOrder(order);
    }
    else
    {
        matchSellOrder(order);
    }

    // Remaining quantity is discarded.
}

void OrderBook::addBuyOrder(const Order& order)
{
    auto& ordersAtPrice = buyBook[order.price];

    ordersAtPrice.push_back(order);

    orderIndex[order.id] =
    {
        Side::Buy,
        order.price,
        std::prev(ordersAtPrice.end())
    };
}

void OrderBook::addSellOrder(const Order& order)
{
    auto& ordersAtPrice = sellBook[order.price];

    ordersAtPrice.push_back(order);

    orderIndex[order.id] =
    {
        Side::Sell,
        order.price,
        std::prev(ordersAtPrice.end())
    };
}