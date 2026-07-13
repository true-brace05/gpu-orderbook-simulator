#include "OrderBook.h"

#include <algorithm>
#include <iomanip>
#include <iostream>

void OrderBook::addOrder(const Order& order)
{
    if (order.quantity <= 0)
    {
        std::cerr << "Invalid quantity.\n";
        return;
    }

    if (order.price <= 0)
    {
        std::cerr << "Invalid price.\n";
        return;
    }

    Order workingOrder = order;

    if (workingOrder.side == Side::Buy)
    {
        matchBuyOrder(workingOrder);

        if (workingOrder.quantity > 0)
        {
            auto& ordersAtPrice = buyBook[workingOrder.price];
            ordersAtPrice.push_back(workingOrder);
            orderIndex[workingOrder.id] = {
                Side::Buy,
                workingOrder.price,
                std::prev(ordersAtPrice.end())};
        }
    }
    else
    {
        matchSellOrder(workingOrder);

        if (workingOrder.quantity > 0)
        {
            auto& ordersAtPrice = sellBook[workingOrder.price];
            ordersAtPrice.push_back(workingOrder);
            orderIndex[workingOrder.id] = {
                Side::Sell,
                workingOrder.price,
                std::prev(ordersAtPrice.end())};
        }
    }
}

void OrderBook::matchBuyOrder(Order& order)
{
    while (order.quantity > 0 && !sellBook.empty())
    {
        auto bestSell = sellBook.begin();

        if (bestSell->first > order.price)
        {
            break;
        }

        Order& restingOrder = bestSell->second.front();
        const int tradeQuantity = std::min(order.quantity, restingOrder.quantity);

        order.quantity -= tradeQuantity;
        restingOrder.quantity -= tradeQuantity;

        std::cout << "TRADE -> " << tradeQuantity << " @ " << bestSell->first << '\n';

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

        if (bestBuy->first < order.price)
        {
            break;
        }

        Order& restingOrder = bestBuy->second.front();
        const int tradeQuantity = std::min(order.quantity, restingOrder.quantity);

        order.quantity -= tradeQuantity;
        restingOrder.quantity -= tradeQuantity;

        std::cout << "TRADE -> " << tradeQuantity << " @ " << bestBuy->first << '\n';

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
