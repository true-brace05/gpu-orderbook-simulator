#pragma once
#include "execution/ITradeListener.h"
class Portfolio : public ITradeListener
{
private:

    int position = 0;

    double cash = 0.0;

public:

    void onBuy(double price, int quantity);

    void onSell(double price, int quantity);

    void onTrade(const Trade& trade) override;

    int getPosition() const;

    double getCash() const;
};