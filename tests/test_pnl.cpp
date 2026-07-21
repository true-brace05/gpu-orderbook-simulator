#include <cassert>
#include <iostream>

#include "portfolio/Portfolio.h"
#include "execution/Fill.h"

int main()
{
    Portfolio portfolio;

    portfolio.onFill({Side::Buy, 100.0, 10, 1});

    assert(portfolio.getPosition() == 10);
    assert(portfolio.getCash() == -1000.0);

    portfolio.onFill({Side::Sell, 120.0, 5, 2});

    assert(portfolio.getPosition() == 5);
    assert(portfolio.getRealizedPnL() == 100.0);

    std::cout << "PnL test passed!\n";

    return 0;
}