#pragma once

#include <vector>

struct PriceLevel
{
    double price = 0.0;
    int quantity = 0;
};

struct OrderBookSnapshot
{
    std::vector<PriceLevel> bids;
    std::vector<PriceLevel> asks;
};