#pragma once

#include <cstdint>

struct Trade
{
    int buyOrderId;

    int sellOrderId;

    double price;

    int quantity;

    uint64_t timestamp;
};