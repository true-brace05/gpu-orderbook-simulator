#pragma once

#include "strategy/IStrategy.h"

class PassiveBuyer : public IStrategy
{
public:
    std::vector<Order> onEvent(
        const Event& event,
        const OrderBook& book
    ) override;
};
