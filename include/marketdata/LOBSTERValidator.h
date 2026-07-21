#pragma once

#include "marketdata/OrderBookSnapshot.h"

class LOBSTERValidator
{
public:
    static bool validate(
        const OrderBookSnapshot& ours,
        const OrderBookSnapshot& expected);

    static void printDifferences(
        const OrderBookSnapshot& ours,
        const OrderBookSnapshot& expected);
};