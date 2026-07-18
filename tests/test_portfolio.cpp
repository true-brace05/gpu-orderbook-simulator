#include <cassert>
#include <iostream>

#include "portfolio/Portfolio.h"

int main()
{
    Portfolio portfolio;

    // Buy 5 @100
    portfolio.onBuy(100, 5);

    assert(portfolio.getPosition() == 5);
    assert(portfolio.getCash() == -500);

    // Sell 2 @105
    portfolio.onSell(105, 2);

    assert(portfolio.getPosition() == 3);
    assert(portfolio.getCash() == -290);

    std::cout << "Portfolio test passed!\n";

    return 0;
}