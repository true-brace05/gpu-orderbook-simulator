#include "strategy/PassiveBuyer.h"

std::vector<Order> PassiveBuyer::onEvent(
    const Event& event,
    const OrderBook& book)
{
    (void)book;

    std::vector<Order> generatedOrders;

    if (event.type != EventType::Add)
    {
        return generatedOrders;
    }

    if (event.order.side != Side::Sell)
    {
        return generatedOrders;
    }

    Order buyOrder;

    buyOrder.id = event.order.id + 1000000;
    buyOrder.side = Side::Buy;
    buyOrder.type = OrderType::Limit;
    buyOrder.price = event.order.price;
    buyOrder.quantity = 1;
    buyOrder.timestamp = event.timestamp;

    generatedOrders.push_back(buyOrder);

    return generatedOrders;
}