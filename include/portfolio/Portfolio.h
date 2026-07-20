#pragma once
#include "execution/IFillListener.h"
class Portfolio : public IFillListener
{
private:
    int position = 0;

    double cash = 0.0;

    double averageEntryPrice = 0.0;

    double realizedPnL = 0.0;

public:

    void onBuy(double price, int quantity);

    void onSell(double price, int quantity);

   void onFill(const Fill& fill) override;

    int getPosition() const;

    double getCash() const;
    double getRealizedPnL() const;
    double getAverageEntryPrice() const;
};