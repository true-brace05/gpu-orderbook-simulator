#include "portfolio/Portfolio.h"

void Portfolio::onBuy(double price, int quantity)
{
    position += quantity;
    cash -= price * quantity;
}

void Portfolio::onSell(double price, int quantity)
{
    position -= quantity;
    cash += price * quantity;
}

int Portfolio::getPosition() const
{
    return position;
}

double Portfolio::getCash() const
{
    return cash;
}

#include "execution/Trade.h"   // only if not already indirectly included

void Portfolio::onTrade(const Trade& trade)
{
    // For now, assume this portfolio tracks the BUY side.
    // We'll improve this design later.

    onBuy(trade.price, trade.quantity);
}