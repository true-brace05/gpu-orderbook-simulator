#include "risk/RiskEngine.h"

#include <cstdlib>

RiskEngine::RiskEngine(const Portfolio& portfolio,
                       const RiskLimits& limits)
    : portfolio(portfolio),
      limits(limits)
{
}

bool RiskEngine::approve(const Order& order) const
{
    // Rule 1: Quantity must be positive
    if (order.quantity <= 0)
    {
        return false;
    }

    // Rule 2: Price must be positive
    if (order.price <= 0.0)
    {
        return false;
    }

    // Rule 3: Order size limit
    if (order.quantity > limits.maxOrderSize)
    {
        return false;
    }

    // Rule 4: Position limit
    int projectedPosition = portfolio.getPosition();

    if (order.side == Side::Buy)
    {
        projectedPosition += order.quantity;
    }
    else
    {
        projectedPosition -= order.quantity;
    }

    return std::abs(projectedPosition) <= limits.maxPosition;
}