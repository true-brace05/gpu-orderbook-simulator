#include <iostream>
#include "OrderBook.h"

void OrderBook::addOrder(const Order& order)
{
    Order workingOrder = order;

    if (workingOrder.side == Side::Buy)
    {
        buyBook[workingOrder.price].push_back(workingOrder);
    }
    else
    {
        sellBook[workingOrder.price].push_back(workingOrder);
    }
}

void OrderBook::cancelOrder(int orderId)
{
    // TODO: Implement in Phase 1
}

void OrderBook::printBook() const
{
    std::cout << "=========== BUY BOOK ===========" << std::endl;

    for (const auto& priceLevel : buyBook)
    {
        std::cout << "Price: " << priceLevel.first << std::endl;

        for (const auto& order : priceLevel.second)
        {
            std::cout
                << "Order ID: " << order.id
                << " | Quantity: " << order.quantity
                << " | Timestamp: " << order.timestamp
                << std::endl;
        }

        std::cout << std::endl;
    }

    std::cout << "=========== SELL BOOK ===========" << std::endl;

    for (const auto& priceLevel : sellBook)
    {
        std::cout << "Price: " << priceLevel.first << std::endl;

        for (const auto& order : priceLevel.second)
        {
            std::cout
                << "Order ID: " << order.id
                << " | Quantity: " << order.quantity
                << " | Timestamp: " << order.timestamp
                << std::endl;
        }

        std::cout << std::endl;
    }
}