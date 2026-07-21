#include <cassert>
#include <iostream>

#include "Order.h"
#include "portfolio/Portfolio.h"
#include "risk/RiskEngine.h"

int main()
{
    Portfolio portfolio;

    RiskLimits limits;
    limits.maxPosition = 1000;
    limits.maxOrderSize = 500;

    RiskEngine risk(portfolio, limits);

    Order validOrder{
        1,
        Side::Buy,
        OrderType::Limit,
        100.0,
        100,
        1
    };

    Order zeroQuantityOrder{
        2,
        Side::Buy,
        OrderType::Limit,
        100.0,
        0,
        2
    };

    Order negativePriceOrder{
        3,
        Side::Buy,
        OrderType::Limit,
        -100.0,
        100,
        3
    };

    Order hugeOrder{
        4,
        Side::Buy,
        OrderType::Limit,
        100.0,
        10000,
        4
    };

    assert(risk.approve(validOrder));
    assert(!risk.approve(zeroQuantityOrder));
    assert(!risk.approve(negativePriceOrder));
    assert(!risk.approve(hugeOrder));

    std::cout << "Risk Engine test passed!\n";

    return 0;
}