#pragma once

#include "Order.h"
#include "portfolio/Portfolio.h"

struct RiskLimits
{
    int maxPosition = 1000;
    int maxOrderSize = 500;
};

class RiskEngine
{
public:
    RiskEngine(const Portfolio& portfolio,
               const RiskLimits& limits = RiskLimits());

    bool approve(const Order& order) const;

private:
    const Portfolio& portfolio;
    RiskLimits limits;
};