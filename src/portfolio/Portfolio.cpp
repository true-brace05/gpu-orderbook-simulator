#include "portfolio/Portfolio.h"

void Portfolio::onBuy(double price, int quantity)
{
    if (position == 0)
    {
        averageEntryPrice = price;
    }
    else
    {
        averageEntryPrice =
            ((averageEntryPrice * position) + (price * quantity))
            / (position + quantity);
    }

    position += quantity;
    cash -= price * quantity;
}

void Portfolio::onSell(double price, int quantity)
{
    realizedPnL += (price - averageEntryPrice) * quantity;

    position -= quantity;
    cash += price * quantity;

    if (position == 0)
    {
        averageEntryPrice = 0.0;
    }
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

void Portfolio::onFill(const Fill& fill)
{
    if (fill.side == Side::Buy)
    {
        onBuy(fill.price, fill.quantity);
    }
    else
    {
        onSell(fill.price, fill.quantity);
    }
}

double Portfolio::getRealizedPnL() const
{
    return realizedPnL;
}
double Portfolio::getAverageEntryPrice() const
{
    return averageEntryPrice;
}