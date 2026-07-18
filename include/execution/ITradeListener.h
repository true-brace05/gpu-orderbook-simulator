#pragma once

#include "execution/Trade.h"

class ITradeListener
{
public:
    virtual ~ITradeListener() = default;

    virtual void onTrade(const Trade& trade) = 0;
};