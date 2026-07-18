#pragma once

#include <vector>

#include "Order.h"
#include "OrderBook.h"
#include "replay/Event.h"

class IStrategy
{
    private:

    IStrategy* strategy = nullptr;

public:

    void setStrategy(IStrategy* newStrategy);
public:
    virtual ~IStrategy() = default;

    virtual std::vector<Order> onEvent(
        const Event& event,
        const OrderBook& book
    ) = 0;
};